#pragma once

#include <vector>
#include <complex>

namespace WaveformMath
{

class FFTprocessor
{
private:
	static int nextPowerOfTwo(int n);
	static void fft(std::vector<std::complex<float>>& data);

public:
	// ctor / dtor
	FFTprocessor();
	~FFTprocessor() = default;

	// runs a "continuous" (short-time) FFT across waveform: frameCount snapshots taken at
	// evenly spaced hops across the whole signal, each reporting the magnitude of its
	// first binCount frequency bins. result[frameIdx][binIdx].
	static std::vector<std::vector<float>> computeSTFT(const std::vector<float>& waveform, int frameCount, int binCount);
};

}
