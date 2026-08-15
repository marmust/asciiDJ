#include <FFTprocessor.hpp>

#include <cmath>
#include <algorithm>

namespace WaveformMath
{

/// @brief ctor
FFTprocessor::FFTprocessor() {}

/// @brief rounds n up to the next power of two (1 for n <= 1)
int FFTprocessor::nextPowerOfTwo(int n)
{
	int power = 1;
	while (power < n)
		power *= 2;

	return power;
}

/// @brief in-place iterative radix-2 Cooley-Tukey FFT
/// @param std::vector<std::complex<float>>& data the samples to transform in place, size must be a power of two
void FFTprocessor::fft(std::vector<std::complex<float>>& data)
{
	size_t n = data.size();
	if (n <= 1)
		return;

	// bit-reversal permutation
	for (size_t i = 1, j = 0; i < n; i++)
	{
		size_t bit = n >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;

		if (i < j)
			std::swap(data[i], data[j]);
	}

	// butterfly stages
	for (size_t len = 2; len <= n; len <<= 1)
	{
		float angle = -2.0f * (float)M_PI / (float)len;
		std::complex<float> wlen(std::cos(angle), std::sin(angle));

		for (size_t i = 0; i < n; i += len)
		{
			std::complex<float> w(1.0f, 0.0f);

			for (size_t k = 0; k < len / 2; k++)
			{
				std::complex<float> u = data[i + k];
				std::complex<float> v = data[i + k + len / 2] * w;

				data[i + k] = u + v;
				data[i + k + len / 2] = u - v;

				w *= wlen;
			}
		}
	}
}

/// @brief runs a short-time FFT sequence over waveform, producing frameCount snapshots of
/// binCount frequency-magnitude bins each, hopping evenly across the whole waveform
/// @param std::vector<float> waveform the input samples
/// @param int frameCount how many FFT frames to produce across waveform
/// @param int binCount how many frequency bins each frame should report
/// @returns std::vector<std::vector<float>> result[frameIdx][binIdx], empty if inputs are degenerate
std::vector<std::vector<float>> FFTprocessor::computeSTFT(const std::vector<float>& waveform, int frameCount, int binCount)
{
	std::vector<std::vector<float>> result;

	if (waveform.empty() || frameCount <= 0 || binCount <= 0)
		return result;

	result.reserve((size_t)frameCount);

	// fftSize must be a power of two; the lower half of its spectrum is the usable part
	// for a real-valued input (the upper half just mirrors it), so size it to cover
	// at least binCount bins there
	int fftSize = nextPowerOfTwo(binCount * 2);

	int hopSize = std::max(1, (int)waveform.size() / frameCount);

	for (int frame = 0; frame < frameCount; frame++)
	{
		size_t windowStart = (size_t)frame * (size_t)hopSize;

		std::vector<std::complex<float>> window(fftSize, std::complex<float>(0.0f, 0.0f));

		for (int i = 0; i < fftSize; i++)
		{
			size_t sampleIdx = windowStart + (size_t)i;
			if (sampleIdx < waveform.size())
				window[i] = std::complex<float>(waveform[sampleIdx], 0.0f);
			// else stays zero-padded
		}

		fft(window);

		std::vector<float> bins;
		bins.reserve((size_t)binCount);

		for (int b = 0; b < binCount; b++)
			bins.push_back(std::abs(window[b]));

		result.push_back(std::move(bins));
	}

	return result;
}

}
