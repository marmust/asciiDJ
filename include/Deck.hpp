#pragma once

#include <vector>
#include <atomic>
#include <string>

#include "miniaudio.h"
#include <InputTuning.hpp>

namespace Playback
{

struct DeckParams
{
	std::atomic<ma_double> playbackSpeed = Input::SPEED_START_VAL;
	std::atomic<ma_double> turntableShift = Input::SHIFT_START_VAL;
	std::atomic<ma_double> volume = 1.0;

	// copy ctor / assignment spelled out since std::atomic is neither copyable nor assignable
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
	// signed so a backwards scratch can move the playhead before the track start (reads as silence)
	std::atomic<ma_int64> cursor = 0;
	std::string name;

	// typical loud-passage level (mean |sample|, mono) the waveform trace normalizes against, set on load
	float waveformReference = 0.0f;

	// ride along because we can only access this struct in the data_callback()
	DeckParams params;

	// for input interpolation
	DeckParams trailingParams;
};

class Deck
{
public:
	// fixed format every track is decoded into, shared by every Deck and the Mixer's device
	static constexpr ma_uint32 kOutputChannels = 2;
	static constexpr ma_uint32 kOutputSampleRate = 48000;

	// audio callback period for every Deck and the Mixer's device (a hint, the backend may clamp it)
	static constexpr ma_uint32 kPeriodSizeMilliseconds = 1;

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
	void computeWaveformReference();
	static float monoSampleAt(const Track& track, ma_uint64 frame);

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

	// read from deckParams, so they stay valid while no track is loaded
	double getPlaybackSpeed() const;
	double getTurntableShift() const;
};

}
