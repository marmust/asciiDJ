#include <Mixer.hpp>
#include <InputTuning.hpp>

#include <algorithm>

namespace Playback
{

/// @brief ctor
Mixer::Mixer() {}

/// @brief adds a deck to mix from
/// @param Deck* newDeck non-owning deck ptr to mix from
/// @param Equalizer* eq externally owned EQ to be applied over that deck
void Mixer::addDeck(Deck* newDeck, Equalizer* eq)
{
	// no format check needed, every deck decodes into Deck's fixed output format

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
        deviceConfig.playback.channels = Deck::kOutputChannels;
        deviceConfig.sampleRate        = Deck::kOutputSampleRate;
        deviceConfig.dataCallback      = this->data_callback;
        // pUserData is a single void*, so decks and crossfader travel as a pair
        this->callbackData = { &this->decks, &this->crossfader };
        deviceConfig.pUserData         = &this->callbackData;
        deviceConfig.periodSizeInMilliseconds = Deck::kPeriodSizeMilliseconds;

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

/// @brief miniaudio audio thread callback, mixes every deck's output according to mixer params
/// @param you get the picture
void Mixer::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	// dst of data (miniaudio's driver wrapper)
	float* out = static_cast<float*>(pOutput);

	// unpack the decks vector and crossfader value handed over by play()
	auto* callbackData = static_cast<std::pair<std::vector<std::unique_ptr<DeckRidealong>>*, double*>*>(pDevice->pUserData);
	std::vector<std::unique_ptr<DeckRidealong>>* deckvec = callbackData->first;
	double crossfader = *callbackData->second;
	double normedXfader = (crossfader + 1.0) / 2.0;

	// even decks sit on the left of the crossfader, odd decks on the right
	int currentTrack = 0;

	// request from each deck his share of audio (decks' callback), and mix according to mixer params
	for (auto it = deckvec->begin(); it != deckvec->end(); it++)
	{
		// skip if deck track is unloaded
		Track* deckTrack = (*it)->deck->getLoadedTrack();

		if (!deckTrack)
			continue;

		// create a dummy device only to carry over the deckTrack
		ma_device dummyDevice{};
		dummyDevice.pUserData = deckTrack;

		// frames are interleaved, so the buffer holds frameCount * channels floats
		ma_uint32 channels = deckTrack->channels;
		float* currentExtracted = new float[(size_t)frameCount * channels]{};

		// get the deck to copy over its data into currentExtracted
		(*it)->deck->data_callback(&dummyDevice, currentExtracted, nullptr, frameCount);

		// mix currentExtracted with the miniaudio output, sample by interleaved sample
		for (ma_uint32 sampleIdx = 0; sampleIdx < frameCount * channels; sampleIdx++)
		{
			// TODO: channels share one filter history per band, so the EQ bleeds L/R
			(*it)->eq->applyEQ(&currentExtracted[sampleIdx]);

			// apply volume
			currentExtracted[sampleIdx] *= (float)(*it)->volume;

			// apply crossfader as volume (invert on odd decks)
			currentExtracted[sampleIdx] *= currentTrack % 2 == 0 ? 1 - normedXfader : normedXfader;

			out[sampleIdx] += currentExtracted[sampleIdx];
		}

		delete[] currentExtracted;
		currentTrack++;
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

/// @brief setter for an individual deck's volume
/// @param double volume volume to be set [0, 1]
/// @param int deckIdx which deck to set the volume for (0 = first added to mixer)
void Mixer::setVolume(double volume, int deckIdx)
{
	// no op if idx out of range
	if (deckIdx < 0 || deckIdx >= decks.size())
		return;

	this->decks[deckIdx]->volume = volume;
}

/// @brief setter for an individual deck's volume
/// @param double volume volume to be set [0, 1]
/// @param int deckIdx which deck to set the volume for (0 = first added to mixer)
void Mixer::setXfader(double xfaderPos)
{
	// clamp to supported range
	xfaderPos = std::clamp(xfaderPos, Input::XFADER_MIN, Input::XFADER_MAX);

	this->crossfader = xfaderPos;
}

/// @brief accessor for an individual deck's volume
/// @param int deckIdx which deck to read the volume of (0 = first added to mixer)
/// @returns double the deck's volume, 0.0 if idx out of range
double Mixer::getVolume(int deckIdx) const
{
	if (deckIdx < 0 || deckIdx >= decks.size())
		return 0.0;

	return this->decks[deckIdx]->volume;
}

/// @brief accessor for the crossfader position
double Mixer::getXfader() const
{
	return this->crossfader;
}

}
