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

struct Track
{
	// track data / metadata
	std::vector<float> frames;
	ma_uint64 frameCount = 0;
	ma_uint32 channels = 0;
	ma_uint32 sampleRate = 0;
	std::atomic<ma_uint64> cursor = 0;

	// ride along because we can only access this struct in the data_callback()
	std::atomic<ma_double> playbackSpeed = 1.0;
	std::atomic<ma_double> turntableShift = 0.0;

	std::atomic<ma_double> volume = 1.0;
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
	std::atomic<ma_double> playbackSpeed = 1.0;
	double previousPlaybackSpeed = 1.0;
	std::atomic<ma_double> turntableShift = 0.0;
	double previousTurntableShift = 0.0;
	std::atomic<ma_double> volume = 1.0;

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
};

}
