#include <Deck.hpp>
#include <WaveformTuning.hpp>

#include <filesystem>
#include <algorithm>
#include <cmath>

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
	// setup a temporary miniaudio decoder - fixed output channels/sampleRate rather than 0/0
	// (native) so every loaded track ends up in the same canonical format, decoder does the
	// resampling/remixing internally
	ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, kOutputChannels, kOutputSampleRate);
	ma_decoder decoder;

	if (ma_decoder_init_file(fpath.c_str(), &decoderConfig, &decoder) != MA_SUCCESS)
		// possibly add a warning system through TUI later
		return;

	// tear down a live standalone device first: it may still be reading loadedTrack on its own
	// thread, and resetTrackMetadata() below is about to mutate that data out from under it. also
	// means playStandalone() never has to reinit for a new track's format - format is now always
	// the same canonical one, so a fresh init is only ever needed the first time it's started
	this->stopStandalone();

	// wipe any previously loaded track's audio data/metadata first - loadTrack must be safe to
	// call on a deck that already has a track loaded (e.g. picking a new file from a browser),
	// and the frame-extraction loop below appends rather than replaces
	this->resetTrackMetadata();

	// setup metadata
	this->loadedTrack.channels = decoder.outputChannels;
	this->loadedTrack.sampleRate = decoder.outputSampleRate;
	this->loadedTrack.name = std::filesystem::path(fpath).filename().string();

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
	this->computeWaveformReference();
	this->isLoaded = true;

	ma_decoder_uninit(&decoder);

	// reset cursor/params to fresh defaults, then land the deck paused at the start of the fresh
	// track rather than mid-track and playing. explicit here rather than relying on
	// stopStandalone() above for this - it only resets as a side effect of actually having been
	// playing, and is a no-op otherwise (e.g. driven by a Mixer instead, which never touches
	// isStandalonePlaying)
	this->resetTrack();
	this->pause();
}

/// @brief helper to wipe the loaded track's audio data and metadata (frames, channels, sampleRate, frameCount)
void Deck::resetTrackMetadata()
{
	this->loadedTrack.frames = {};
	this->loadedTrack.channels = 0;
	this->loadedTrack.sampleRate = 0;
	this->loadedTrack.frameCount = 0;
	this->loadedTrack.name = "";
	this->loadedTrack.waveformReference = 0.0f;
}

/// @brief mixes one frame of a track down to mono by averaging its channels
/// @param Track track the track to read from
/// @param ma_uint64 frame the frame index, must be < track.frameCount
/// @returns float the mono sample, 0 for a track with no channels
float Deck::monoSampleAt(const Track& track, ma_uint64 frame)
{
	if (track.channels == 0)
		return 0.0f;

	float sum = 0.0f;
	for (ma_uint32 ch = 0; ch < track.channels; ch++)
		sum += track.frames[(size_t)frame * track.channels + ch];

	return sum / (float)track.channels;
}

/// @brief measures the loaded track's mean |sample| (mono) over consecutive blocks and stores the
/// Waveform::REFERENCE_PERCENTILE-th block level as the track's waveformReference, the fixed level
/// the waveform trace normalizes against
void Deck::computeWaveformReference()
{
	Track* track = &this->loadedTrack;

	std::vector<float> blockLevels;
	blockLevels.reserve(track->frameCount / Waveform::AVERAGING_FRAMES + 1);

	for (ma_uint64 blockStart = 0; blockStart < track->frameCount; blockStart += Waveform::AVERAGING_FRAMES)
	{
		ma_uint64 blockEnd = std::min<ma_uint64>(track->frameCount, blockStart + Waveform::AVERAGING_FRAMES);

		float sum = 0.0f;
		for (ma_uint64 frame = blockStart; frame < blockEnd; frame++)
			sum += std::abs(monoSampleAt(*track, frame));

		blockLevels.push_back(sum / (float)(blockEnd - blockStart));
	}

	if (blockLevels.empty())
	{
		track->waveformReference = 0.0f;
		return;
	}

	size_t percentileIdx = (size_t)(Waveform::REFERENCE_PERCENTILE * (double)(blockLevels.size() - 1));
	std::nth_element(blockLevels.begin(), blockLevels.begin() + percentileIdx, blockLevels.end());
	track->waveformReference = blockLevels[percentileIdx];
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

	// flatten the interpolation ramp too, otherwise the next audio callback lerps from whatever
	// params were trailing before this reset (e.g. leftover from a track loaded onto this deck
	// previously) instead of starting flat at the just-reset values
	this->loadedTrack.trailingParams = this->loadedTrack.params;
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

	// update the struct to the new cursor position, clamped to the track's limits so a
	// backwards scratch parks at the start instead of running away before it. the upper bound
	// is frameCount rather than frameCount - 1 on purpose: "at or past the end" has to stay
	// representable for getTrackProgress/getTimeRemainingSeconds to read 100% and 0:00.
	// note this clamps only what gets PERSISTED - currentSample walks unclamped through the
	// block above so the silence guard still decides per sample whether there is audio there
	track->cursor = (ma_int64)std::clamp(currentSample, 0.0, (double)track->frameCount);

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

/// @brief extracts a window of track audio centered on the cursor, mono-mixed across channels
/// (one float per output sample) for use as a single waveform trace. The window spans
/// +-windowSeconds of *replay* time: frames are sampled every |playbackSpeed| track frames, so the
/// returned array is always the same length but covers more track audio the faster the deck plays
/// (the trace squishes/expands with speed instead of scrolling faster/slower). Turntable shift is
/// deliberately ignored so scratches don't warp the trace. Positions outside the track are silence.
/// @returns std::vector<float> the extracted waveform, empty if no track is loaded
std::vector<float> Deck::extractExpectedWaveform(double windowSeconds)
{
	if (!this->isLoaded)
		return {};

	Track* track = &this->loadedTrack;

	// +-windowSeconds of replay time around the cursor
	long halfWindowFrames = (long)(windowSeconds * track->sampleRate);
	long totalFrames = halfWindowFrames * 2;

	// step through the track at the playback speed's rate, same multiplier data_callback advances
	// by. abs() keeps the trace reading left-to-right in reverse
	double stride = std::max(std::abs((double)track->params.playbackSpeed), Waveform::MIN_STRIDE);

	std::vector<float> waveform;
	waveform.reserve((size_t)totalFrames);

	double cursor = (double)track->cursor;

	for (long i = 0; i < totalFrames; i++)
	{
		long sampleIndex = (long)(cursor + (double)(i - halfWindowFrames) * stride);

		bool inTrack = sampleIndex >= 0 && (ma_uint64)sampleIndex < track->frameCount;
		waveform.push_back(inTrack ? monoSampleAt(*track, (ma_uint64)sampleIndex) : 0.0f);
	}

	return waveform;
}

/// @brief external accessor for whether the deck is currently paused
bool Deck::getIsPaused() const
{
	return this->isPaused;
}

/// @brief external accessor for whether the deck's own standalone device is currently running
bool Deck::getIsStandalonePlaying() const
{
	return this->isStandalonePlaying;
}

/// @brief external accessor for the deck's live playback speed, meaningful whether or not a
/// track is loaded (0.0 while paused/unloaded, same value the audio callback actually uses)
double Deck::getPlaybackSpeed() const
{
	return this->deckParams.playbackSpeed;
}

/// @brief external accessor for the deck's live turntable shift, meaningful whether or not a
/// track is loaded (0.0 while paused/unloaded, same value the audio callback actually uses)
double Deck::getTurntableShift() const
{
	return this->deckParams.turntableShift;
}

/// @brief external accessor for the loaded track's playback progress
/// @returns float progress in [0, 1], 0 if no track is loaded
float Deck::getTrackProgress() const
{
	if (!this->isLoaded || this->loadedTrack.frameCount == 0)
		return 0.0f;

	float progress = (float)this->loadedTrack.cursor / (float)this->loadedTrack.frameCount;
	return std::clamp(progress, 0.0f, 1.0f);
}

/// @brief external accessor for the loaded track's remaining content duration; ignores current
/// playback speed (same content-position basis as getTrackProgress, not a wall-clock ETA)
/// @returns double seconds remaining, 0 if no track is loaded
double Deck::getTimeRemainingSeconds() const
{
	if (!this->isLoaded || this->loadedTrack.sampleRate == 0)
		return 0.0;

	ma_int64 cursor = this->loadedTrack.cursor;
	ma_int64 frameCount = (ma_int64)this->loadedTrack.frameCount;

	// clamped at both ends: past the end nothing is left, and scratched back before the start
	// there is still only a whole track ahead, not more than one
	ma_int64 remainingFrames = std::clamp(frameCount - cursor, (ma_int64)0, frameCount);

	return (double)remainingFrames / (double)this->loadedTrack.sampleRate;
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

/// @brief function to pause a currently playing track (keeps cursor pos and turntable shift),
/// independent of whether the deck's audio is being pulled by its own standalone device or by a
/// mixer. only playbackSpeed freezes - turntableShift is left alone so the deck can still be
/// scratched while paused, same as a real turntable's platter
void Deck::pause()
{
	// no op if already paused
	if (this->isPaused)
		return;

	this->previousPlaybackSpeed = this->deckParams.playbackSpeed;
	this->deckParams.playbackSpeed = 0.0;
	this->syncModifiersToTrack();
	this->isPaused = true;

	// DO NOT reset track params so cursor & deck mods stay the same
}

/// @brief function to unpause and keep playing the loaded track from the position left off when
/// paused, restoring whatever playback speed was active before the pause
void Deck::unpause()
{
	// no op if not paused
	if (!this->isPaused)
		return;

	this->deckParams.playbackSpeed = this->previousPlaybackSpeed;
	this->syncModifiersToTrack();
	this->isPaused = false;
}

/// @brief setter for the turn table shift (record scratch) - always live, paused or not, so
/// scratching still moves the cursor while playbackSpeed is frozen at 0
/// @param double shift new shift
void Deck::setTurntableShift(double shift)
{
	this->deckParams.turntableShift = shift;
	this->syncModifiersToTrack();
}

/// @brief setter for the turn table speed - touching this while paused means "make it move
/// again", so it unpauses first rather than only parking the value for a later manual unpause
/// @param double speed new speed
void Deck::setPlaybackSpeed(double speed)
{
	this->isPaused = false;

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
