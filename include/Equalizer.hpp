#pragma once

#include <vector>
#include <mutex>

namespace Playback
{

struct EQmemory
{
        float x1 = 0.0;
        float x2 = 0.0;
        float y1 = 0.0;
        float y2 = 0.0;
};

struct EQsetup
{
        float a1 = 0.0;
        float a2 = 0.0;
        float b0 = 1.0;
        float b1 = 0.0;
        float b2 = 0.0;
};

struct EQparams : public EQmemory, public EQsetup
{
	// the gain (dB) computeIIRparams() was last called with for this band, kept around
	// purely for telemetry - the biquad coefficients above are what's actually applied
	float gainDB = 0.0;
};

class Equalizer
{
private:
	std::vector<EQparams> bands;
	std::mutex bandsMtx;

	int sampleRate = 0;
	bool sampleRateReported = false;

	void computeSingleBand(EQparams* band, float* sample);
	void stackBands(std::vector<EQparams>* bands, float* sample); // lol

	EQparams computeIIRparams(int sampleRate, float freq, float q, float gainDB);

public:
	// ctor / dtor
	Equalizer();
	~Equalizer() = default;

	// eq passthru
	void applyEQ(float* sample);

	void reportSampleRate(int sampleRate);

	void addBand(float freq, float q, float gainDB);
	void changeBand(int bandIdx, float freq, float q, float gainDB);
	void deleteBand(int bandIdx);
	void resetEQ();

	float getBandGain(int bandIdx);
};

}
