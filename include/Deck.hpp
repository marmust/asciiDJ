#pragma once

#include <vector>
#include <atomic>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>

#include "miniaudio.h"

namespace Playback
{

struct DeckParams
{
	std::atomic<ma_double> playbackSpeed = 1.0;
	std::atomic<ma_double> turntableShift = 0.0;
	std::atomic<ma_double> volume = 1.0;

	// std::atomic itself is neither copyable nor assignable, so DeckParams needs these spelled
	// out by hand (loading/storing the underlying value rather than copying the atomics themselves)
	DeckParams() = default;

	DeckParams(const DeckParams& other)
		: playbackSpeed(other.playbackSpeed.load()),
		  turntableShift(other.turntableShift.load()),
		  volume(other.volume.load())
	{}

	DeckParams& operator=(const DeckParams& other)
	{
		this->playbackSpeed.store(other.playbackSpeed.load());
		this->turntableShift.store(other.turntableShift.load());
		this->volume.store(other.volume.load());
		return *this;
	}
};

struct Track
{
	// track data / metadata
	std::vector<float> frames;
	ma_uint64 frameCount = 0;
	ma_uint32 channels = 0;
	ma_uint32 sampleRate = 0;
	std::atomic<ma_uint64> cursor = 0;

	// ride along because we can only access this struct in the data_callback()
	DeckParams params;

	// for input interpolation
	DeckParams trailingParams;
};

class Deck
{
private:
	// track frame array
	Track loadedTrack;
	bool isLoaded = false;
	bool isStandalonePlaying = false;
	bool isPaused = false;

	// track progress and modifiers
	DeckParams deckParams;

	// for pausing / unpausing
	double previousPlaybackSpeed = 1.0;
	double previousTurntableShift = 0.0;

	// miniaudio backend
	ma_device device;

	void syncModifiersToTrack();
	void resetTrackMetadata();

public:
	// ctor / dtor
	Deck();
	~Deck();

	// track management
	void loadTrack(std::string fpath);
	void unloadTrack();

	void resetTrack();

	void playStandalone();
	void stopStandalone();
	void pause();
	void unpause();

	// temp, should be on the mixer
	void setStandaloneVolume(double volume);

	// control
	void setTurntableShift(double shift);
	void setPlaybackSpeed(double speed);

	// miniaudio comms
	static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
	Track* getLoadedTrack();

	// rendering
	std::vector<float> extractExpectedWaveform(double windowSeconds);

	// state accessors
	bool getIsPaused() const;
	bool getIsStandalonePlaying() const;
};

}
