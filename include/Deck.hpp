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
	// signed: a backwards scratch can legitimately walk the playhead before the start of the
	// track, and both read paths below already treat a negative index as silence. as an
	// unsigned type the negative double converted straight back to a ~1.8e19 garbage cursor
	std::atomic<ma_int64> cursor = 0;
	std::string name;

	// ride along because we can only access this struct in the data_callback()
	DeckParams params;

	// for input interpolation
	DeckParams trailingParams;
};

class Deck
{
public:
	// every track is decoded into this fixed format regardless of its native one (miniaudio's
	// decoder resamples/remixes internally when given explicit non-zero channels/sampleRate), so
	// every Deck and the Mixer's own device always agree on format - no per-load enforcement needed
	static constexpr ma_uint32 kOutputChannels = 2;
	static constexpr ma_uint32 kOutputSampleRate = 48000;

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
	float getTrackProgress() const;
	double getTimeRemainingSeconds() const;

	// live off deckParams directly rather than the loaded track's ridealong copy, so these stay
	// meaningful even when unloaded (0/0, same as a paused deck - startRun() pauses every deck
	// up front regardless of load state) instead of only being readable via getLoadedTrack()
	double getPlaybackSpeed() const;
	double getTurntableShift() const;
};

}
