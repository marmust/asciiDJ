#include <Equalizer.hpp>

#include <cmath>

namespace Playback
{

/// @brief ctor
Equalizer::Equalizer() {}

/// @brief applies a single eq band onto a sample
/// @param EQparams* band the band to apply (writes back to memory)
/// @param float* sample the sample to apply to (modifies)
void Equalizer::computeSingleBand(EQparams* band, float* sample)
{
	float x0 = *sample;

	float y0 = band->b0 * x0
		+ band->b1 * band->x1 + band->b2 * band->x2
		- band->a1 * band->y1 - band->a2 * band->y2;

	band->x2 = band->x1;
	band->x1 = x0;
	band->y2 = band->y1;
	band->y1 = y0;

	*sample = y0;
}

/// @brief function to apply all bands in the EQ to a sample
/// @param float* sample the sample to apply to (modifies)
/// @param std::vector<EQparams>* bands the bands to apply to the sample
void Equalizer::stackBands(std::vector<EQparams>* bands, float* sample)
{
	// iterate over all bands and apply them to the sample
	for (auto it = this->bands.begin(); it != bands->end(); it++)
	{
		//                       v tuff
		this->computeSingleBand(&*it, sample);
	}
}

/// @brief applies the EQ over a sample with the current bands
/// @param float* sample the sample to apply over (modifies)
void Equalizer::applyEQ(float* sample)
{
	// lock mtx
	std::lock_guard<std::mutex> lock(this->bandsMtx);

	this->stackBands(&this->bands, sample);
}

/// @brief computes the biquad coefficients for a peaking (bell) IIR band
/// @param int sampleRate the playback sample rate in Hz
/// @param float freq the band's center frequency in Hz
/// @param float q the band's Q (narrowness of the bell around freq)
/// @param float gainDB the boost/cut applied at the center frequency, in dB
/// @return EQparams a freshly initialized band (zeroed memory) ready to filter samples
EQparams Equalizer::computeIIRparams(int sampleRate, float freq, float q, float gainDB)
{
	float A = std::pow(10.0f, gainDB / 40.0f);
	float w0 = 2.0f * static_cast<float>(M_PI) * freq / static_cast<float>(sampleRate);
	float cosw0 = std::cos(w0);
	float sinw0 = std::sin(w0);
	float alpha = sinw0 / (2.0f * q);

	float a0 = 1.0f + alpha / A;

	EQparams band;

	band.b0 = (1.0f + alpha * A) / a0;
	band.b1 = (-2.0f * cosw0) / a0;
	band.b2 = (1.0f - alpha * A) / a0;
	band.a1 = (-2.0f * cosw0) / a0;
	band.a2 = (1.0f - alpha / A) / a0;

	return band;
}

/// @brief setter for sample rate
/// @param int sampleRate the samplerate this eq is going to work over
void Equalizer::reportSampleRate(int sampleRate)
{
	this->sampleRate = sampleRate;
	this->sampleRateReported = true;
}

/// @brief adds a new band to be applied in the EQ
/// @param float freq frequency to work at
/// @param float q how "wide" the effect around freq is
/// @param float gainDB +- how many decibals to apply on that freq / q range
void Equalizer::addBand(float freq, float q, float gainDB)
{
	// sample rate check
	if (!this->sampleRateReported)
		return;

	// lock mtx
	std::lock_guard<std::mutex> lock(this->bandsMtx);

	EQparams computedBand = this->computeIIRparams(this->sampleRate, freq, q, gainDB);
	computedBand.gainDB = gainDB;

	this->bands.push_back(computedBand);
}

/// @brief changes an existing band in the eq
/// @param int bandIdx which band to change (0 - last added)
/// @param float freq frequency to work at
/// @param float q how "wide" the effect around freq is
/// @param float gainDB +- how many decibals to apply on that freq / q range
void Equalizer::changeBand(int bandIdx, float freq, float q, float gainDB)
{
	// sample rate check
	if (!this->sampleRateReported)
		return;

	// lock mtx
	std::lock_guard<std::mutex> lock(this->bandsMtx);

	// handle range
	if (bandIdx < 0 || bandIdx >= (int)this->bands.size())
		return;

	EQparams computedBand = this->computeIIRparams(this->sampleRate, freq, q, gainDB);
	computedBand.gainDB = gainDB;

	// keep the filter memory, resetting it on every change would keep the filter from ever responding
	EQmemory preservedMemory = this->bands[bandIdx];
	computedBand.x1 = preservedMemory.x1;
	computedBand.x2 = preservedMemory.x2;
	computedBand.y1 = preservedMemory.y1;
	computedBand.y2 = preservedMemory.y2;

	this->bands[bandIdx] = computedBand;
}

/// @brief accessor for a band's currently applied gain
/// @param int bandIdx which band to read (0 - last added)
/// @returns float the band's gain in dB, 0.0 if idx out of range
float Equalizer::getBandGain(int bandIdx)
{
	std::lock_guard<std::mutex> lock(this->bandsMtx);

	if (bandIdx < 0 || bandIdx >= this->bands.size())
		return 0.0;

	return this->bands[bandIdx].gainDB;
}

/// @brief delete one of the active bands in the EQ
/// @param int bandIdx which band to delete by idx (last added = 0)
void Equalizer::deleteBand(int bandIdx)
{
	// handle range
	if (bandIdx < 0 || bandIdx >= this->bands.size())
		return;

	this->bands.erase(this->bands.begin() + bandIdx);
}

/// @brief deletes all bands from the EQ
void Equalizer::resetEQ()
{
	this->bands.clear();
}

}
