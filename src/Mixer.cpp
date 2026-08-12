#include <Mixer.hpp>

namespace Playback
{

/// @brief ctor
Mixer::Mixer() {}

/// @brief adds a deck to mix from
/// @param Deck* newDeck non-owning deck ptr to mix from
/// @param Equalizer* eq externally owned EQ to be applied over that deck
void Mixer::addDeck(Deck* newDeck, Equalizer* eq)
{
	// for now only support audio files with idenical sample rates and chunnel numbers
	// TODO: currently this disallows adding unloaded decks, and also to swap the track on a deck another guard needs to be
	// added there. in general there needs to be a big rework (possibly adjusting channels / samplerates?) in this system

	if (!newDeck->getLoadedTrack())
	{
		std::cout << "mixer: for identical metadata enforcement first load a track onto the deck" << std::endl;
		return;
	}

	if (this->decks.size() == 0) // on first deck get the numbers to enforce
	{
		this->allowedSampleRate = newDeck->getLoadedTrack()->sampleRate;
		this->allowedChannelCount = newDeck->getLoadedTrack()->channels;

		std::cout << "this mixer now only allows samplerates: " << this->allowedSampleRate << " channelcounts: " << this->allowedChannelCount << std::endl;
	}

	if (newDeck->getLoadedTrack()->sampleRate != this->allowedSampleRate ||
	    newDeck->getLoadedTrack()->channels != this->allowedChannelCount)
	{
		std::cout << "mixer got mismatched channelcount or samplerate, rejecting deck" << std::endl;
		return;
	}

	// craft a deck ridealong struct (default init params)
	this->decks.push_back(std::make_unique<DeckRidealong>(newDeck, eq));
}

/// @brief func inits a miniaudio device
void Mixer::play()
{
	// no op if already playing
	if (this->isPlaying)
		return;

	        /// config and create the miniaudio device to playback the loaded track
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format   = ma_format_f32;
        deviceConfig.playback.channels = this->allowedChannelCount;
        deviceConfig.sampleRate        = this->allowedSampleRate;
        deviceConfig.dataCallback      = this->data_callback;
        deviceConfig.pUserData         = &this->decks;
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
        this->isPlaying = true;
}

/// @brief callback for miniaudio to use, relies on the callbacks of the decks, returns a mixed frame stream according to mixer params
/// @param you get the picture
void Mixer::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	// dst of data (miniaudio's driver wrapper)
	float* out = static_cast<float*>(pOutput);
	std::vector<std::unique_ptr<DeckRidealong>>* deckvec = static_cast<std::vector<std::unique_ptr<DeckRidealong>>*>(pDevice->pUserData);

	// request from each deck his share of audio (decks' callback), and mix according to mixer params
	for (auto it = deckvec->begin(); it != deckvec->end(); it++)
	{
		// skip if deck track is unloaded
		Track* deckTrack = (*it)->deck->getLoadedTrack();

		if (!deckTrack)
			continue;

		// create a dummy pDevice* (the deck data_callback() only needs the track* from the pDevice's->pUserData)
		// NOTE: must not call ma_device_init here - that opens a real backend stream and is not safe to do
		// from inside another device's audio callback thread (causes reentrant backend calls / heap corruption)
		ma_device dummyDevice{};
		dummyDevice.pUserData = deckTrack;

		// frames/out are interleaved per channel, so the extraction buffer needs frameCount * channels floats
		ma_uint32 channels = deckTrack->channels;
		float* currentExtracted = new float[(size_t)frameCount * channels]{};

		// get the deck to copy over its data into currentExtracted
		(*it)->deck->data_callback(&dummyDevice, currentExtracted, nullptr, frameCount);

		// mix currentExtracted with the miniaudio output, sample by interleaved sample
		for (ma_uint32 sampleIdx = 0; sampleIdx < frameCount * channels; sampleIdx++)
		{
			(*it)->eq->applyEQ(&currentExtracted[sampleIdx]);
			out[sampleIdx] += currentExtracted[sampleIdx] * (float)(*it)->volume;
		}

		delete[] currentExtracted;
	}
}

/// @brief stop and reset all audio in the mixer and its decks
void Mixer::stop()
{
	// no op if already stopped
        if (!this->isPlaying)
                return;

        ma_device_uninit(&this->device);
        this->isPlaying = false;

	// uninit the mixer's device, and reset all the decks
	for (auto it = this->decks.begin(); it != this->decks.end(); it++)
	{
		(*it)->deck->resetTrack();
	}
}

}
