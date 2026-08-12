#include <Deck.hpp>

namespace Playback
{

/// @brief ctor
Deck::Deck() {}

/// @brief dtor
Deck::~Deck()
{
	this->stopStandalone();
	this->unloadTrack();
	this->isLoaded = false;
}

/// @brief helper to copy all the deck's modifiers into the track struct ridealong section
void Deck::syncModifiersToTrack()
{
	this->loadedTrack.params.playbackSpeed = static_cast<ma_double>(this->deckParams.playbackSpeed);
	this->loadedTrack.params.turntableShift = static_cast<ma_double>(this->deckParams.turntableShift);
	this->loadedTrack.params.volume = static_cast<ma_double>(this->deckParams.volume);
}

/// @brief loads an audio file onto the deck, translates into frames in a Track struct
/// @param std::string fpath the filepath to load
void Deck::loadTrack(std::string fpath)
{
	// setup a temporary miniaudio decoder
	ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
	ma_decoder decoder;

	if (ma_decoder_init_file(fpath.c_str(), &decoderConfig, &decoder) != MA_SUCCESS)
		// possibly add a warning system through TUI later
		return;

	// setup metadata
	this->loadedTrack.channels = decoder.outputChannels;
	this->loadedTrack.sampleRate = decoder.outputSampleRate;

	const ma_uint32 CHUNK = 4096;

	// holder for each incoming batch
	std::vector<float> extractedFrames((size_t)CHUNK * this->loadedTrack.channels);

	// extract all file frames into the clip
	while (true)
	{
		ma_uint64 currentFramesRead = 0;
		ma_result result = ma_decoder_read_pcm_frames(&decoder, extractedFrames.data(), CHUNK, &currentFramesRead);

		if (currentFramesRead > 0)
		{
			this->loadedTrack.frames.insert(this->loadedTrack.frames.end(), extractedFrames.begin(), extractedFrames.begin() +
										         (size_t)currentFramesRead * this->loadedTrack.channels);
		}

		if (result != MA_SUCCESS || currentFramesRead < CHUNK)
			break;
	}

	this->loadedTrack.frameCount = this->loadedTrack.frames.size() / this->loadedTrack.channels;
	this->isLoaded = true;

	ma_decoder_uninit(&decoder);
}

/// @brief helper to wipe the loaded track's audio data and metadata (frames, channels, sampleRate, frameCount)
void Deck::resetTrackMetadata()
{
	this->loadedTrack.frames = {};
	this->loadedTrack.channels = 0;
	this->loadedTrack.sampleRate = 0;
	this->loadedTrack.frameCount = 0;
}

/// @brief resets playback position and modifiers back to their defaults, keeps loaded audio intact
void Deck::resetTrack()
{
	// reset params;
	this->loadedTrack.cursor = 0;
	this->deckParams.playbackSpeed = 1.0;
	this->deckParams.turntableShift = 0.0;
	this->deckParams.volume = 1.0;
	this->isPaused = false;

	// push to the active track
	this->syncModifiersToTrack();
}

/// @brief unloads the current track and resets metadata
void Deck::unloadTrack()
{
	// unload the audio itself along with resetting all the metadata
	this->resetTrackMetadata();

	// and reset playback position/modifiers on top of that
	this->resetTrack();
}

/// @brief a func to be called upon by the miniaudio's audio thread, reads track and sends a batch to miniaudio
/// @param idk u asking me?
void Deck::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	// source of data (track) and dst of data (miniaudio's driver wrapper)
	Track* track = static_cast<Track*>(pDevice->pUserData);
	float* out = static_cast<float*>(pOutput);

	// fill the requested number of frames into the output

	int currentlyTransferred = 0;
	double currentSample = (double)track->cursor; // is double because speed control is analog

	// snapshot of the live params at the start of this callback, DeckParams' copy ctor
	// handles the atomic members (a plain brace-init no longer works now that it has one)
	DeckParams interpolationParams = track->params;

	// BuT A fOr LoOp iS tHe CorReCT oNe
	while (currentlyTransferred < frameCount)
	{
		// linear input interpolation
		double lerpProgress = (double)currentlyTransferred / (double)frameCount;

		interpolationParams.playbackSpeed = track->trailingParams.playbackSpeed * (1.0 - lerpProgress) +
						     track->params.playbackSpeed * lerpProgress;

		interpolationParams.turntableShift = track->trailingParams.turntableShift * (1.0 - lerpProgress) +
						      track->params.turntableShift * lerpProgress;

		interpolationParams.volume = track->trailingParams.volume * (1.0 - lerpProgress) +
					      track->params.volume * lerpProgress;

		long sampleIndex = (long)currentSample;

		// past end of track: output silence instead of reading out of bounds
		if (sampleIndex < 0 || (ma_uint64)sampleIndex >= track->frameCount)
		{
			for (ma_uint32 ch = 0; ch < track->channels; ch++)
				out[currentlyTransferred * track->channels + ch] = 0.0f;
		}
		else
		{
			// frames/out are interleaved per channel, so index accordingly
			for (ma_uint32 ch = 0; ch < track->channels; ch++)
				out[currentlyTransferred * track->channels + ch] = track->frames[(size_t)sampleIndex * track->channels + ch] * interpolationParams.volume;
		}

		// move the cursor according to the settings
		currentSample += interpolationParams.playbackSpeed + interpolationParams.turntableShift;
		currentlyTransferred++;
	}

	// update the struct to the new cursor position
	track->cursor = (ma_uint64)currentSample;

	// update interpolation for next cycle
	track->trailingParams = track->params;
}

/// @brief external accessor to the loaded track struct (used to give back to this instance's callback in the mixer)
/// @returns Track* to currently loaded track struct (nullptr if unloaded)
Track* Deck::getLoadedTrack()
{
	if (!isLoaded)
		return nullptr;

	return &this->loadedTrack;
}

/// @brief function to launch the miniaudio thread and actually play the audio
void Deck::playStandalone()
{
	// no-op if already playing
	if (this->isStandalonePlaying && this->isLoaded)
		return;

	/// config and create the miniaudio device to playback the loaded track
	ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    	deviceConfig.playback.format   = ma_format_f32;
    	deviceConfig.playback.channels = this->loadedTrack.channels;
    	deviceConfig.sampleRate        = this->loadedTrack.sampleRate;
	deviceConfig.dataCallback      = this->data_callback;
	deviceConfig.pUserData         = &this->loadedTrack;
	// default low-latency period is ~10ms; ask for ~10x faster callbacks (a hint - the backend may clamp it)
	deviceConfig.periodSizeInMilliseconds = 1;

	// launch failiure checks
	if (ma_device_init(NULL, &deviceConfig, &this->device) != MA_SUCCESS)
        	return;

	if (ma_device_start(&this->device) != MA_SUCCESS)
	{
	        ma_device_uninit(&this->device);
	        return;
	}

	// device now runs the data_callback on its own thread; this call does not block
	this->isStandalonePlaying = true;
}

/// @brief function to hault the miniaudio player thread if running
void Deck::stopStandalone()
{
	// no op if already stopped (and if never started, there is no device to uninit)
	if (!this->isStandalonePlaying)
		return;

	ma_device_uninit(&this->device);
	this->isStandalonePlaying = false;

	// reset the track without unloading the audio
	this->resetTrack();
}

/// @brief function to pause a currently playing track (keeps cursor pos and turntable mods), independent of
/// whether the deck's audio is being pulled by its own standalone device or by a mixer
void Deck::pause()
{
	// no op if already paused
	if (this->isPaused)
		return;

	// freeze cursor advancement entirely (both speed and turntable shift feed the cursor), remember both for unpause
	this->previousPlaybackSpeed = this->deckParams.playbackSpeed;
	this->previousTurntableShift = this->deckParams.turntableShift;
	this->deckParams.playbackSpeed = 0.0;
	this->deckParams.turntableShift = 0.0;
	this->syncModifiersToTrack();
	this->isPaused = true;

	// DO NOT reset track params so cursor & deck mods stay the same
}

/// @brief function to unpause and keep playing the loaded track from the position left off when paused
void Deck::unpause()
{
	// no op if not paused
	if (!this->isPaused)
		return;

	// restore the playback speed and turntable shift to whatever they were before the pause
	this->deckParams.playbackSpeed = this->previousPlaybackSpeed;
	this->deckParams.turntableShift = this->previousTurntableShift;
	this->syncModifiersToTrack();
	this->isPaused = false;
}

/// @brief setter for the turn table shift (record scratch)
/// @param double shift new shift
void Deck::setTurntableShift(double shift)
{
	this->deckParams.turntableShift = shift;
	this->syncModifiersToTrack();
}

/// @brief setter for the turn table speed
/// @param double speed new speed
void Deck::setPlaybackSpeed(double speed)
{
	this->deckParams.playbackSpeed = speed;
	this->syncModifiersToTrack();
}

/// @brief setter for volume (default=1.0)
/// @param double volume volume to be set
void Deck::setStandaloneVolume(double volume)
{
	this->deckParams.volume = volume;
	this->syncModifiersToTrack();
}

}
