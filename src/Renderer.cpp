#include <Renderer.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Graphics
{

/// @brief renders a horizontal slider string with a holder thing at progress/length location
/// @param int length the total length in chars of the slider
/// @param int progress which char out of length to put the mover on
std::string Renderer::renderHorizontalSlider(int length, int progress)
{
	auto preHandle = std::views::repeat(RenderElements::horizontalSliderRail, progress) | std::views::join;
	auto postHandle = std::views::repeat(RenderElements::horizontalSliderRail, length - progress - 1) | std::views::join;

	return RenderElements::horizontalSliderLeftCap +
	       std::string(preHandle.begin(), preHandle.end()) +
	       RenderElements::horizontalSliderHolder +
	       std::string(postHandle.begin(), postHandle.end()) +
	       RenderElements::horizontalSliderRightCap;
}

/// @brief renders a vertical slider string with a holder thing at progress/length location
/// @param int length the total length (height) in chars of the slider
/// @param int progress which char out of length to put the mover on
std::string Renderer::renderVerticalSlider(int length, int progress)
{
	auto preHandle = std::views::repeat(RenderElements::verticalSliderRail, progress) | std::views::join;
	auto postHandle = std::views::repeat(RenderElements::verticalSliderRail, length - progress - 1) | std::views::join;

	return RenderElements::verticalSliderTopCap +
	       std::string(preHandle.begin(), preHandle.end()) +
	       RenderElements::verticalSliderHolder +
	       std::string(postHandle.begin(), postHandle.end()) +
	       RenderElements::verticalSliderBottomCap;
}

/// @brief renders a horizontal fill bar, filled from the left up to progress out of length
/// @param int length the total length in chars of the bar
/// @param int progress how many chars out of length are filled
std::string Renderer::renderHorizontalFill(int length, int progress)
{
	progress = std::clamp(progress, 0, length);

	auto filled = std::views::repeat(RenderElements::horizontalFillFull, progress) | std::views::join;
	auto empty = std::views::repeat(RenderElements::horizontalFillEmpty, length - progress) | std::views::join;

	return RenderElements::horizontalFillLeftCap +
	       std::string(filled.begin(), filled.end()) +
	       std::string(empty.begin(), empty.end()) +
	       RenderElements::horizontalFillRightCap;
}

/// @brief renders a vertical fill bar, filled from the top up to progress out of length
/// @param int length the total length (height) in chars of the bar
/// @param int progress how many chars out of length are filled
std::string Renderer::renderVerticalFill(int length, int progress)
{
	progress = std::clamp(progress, 0, length);

	auto filled = std::views::repeat(RenderElements::verticalFillFull, progress) | std::views::join;
	auto empty = std::views::repeat(RenderElements::verticalFillEmpty, length - progress) | std::views::join;

	return RenderElements::verticalFillTopCap +
	       std::string(filled.begin(), filled.end()) +
	       std::string(empty.begin(), empty.end()) +
	       RenderElements::verticalFillBottomCap;
}

/// @brief renders the knob frame for a given position
/// @param float position knob position in [-1, 1], +1 = max, -1 = min
std::string Renderer::renderKnob(float position)
{
	position = std::clamp(position, -1.0f, 1.0f);

	int frameCount = static_cast<int>(RenderElements::knobFrames.size());

	int frameIdx = static_cast<int>(std::round((1.0f - position) / 2.0f * (frameCount - 1)));
	frameIdx = std::clamp(frameIdx, 0, frameCount - 1);

	return RenderElements::knobFrames[frameIdx];
}

/// @brief renders the deck turntable for a given spin frame
/// @param int frame which frame to draw, directly indexes RenderElements::turntableFrames
std::string Renderer::renderTurntable(int frame)
{
	int frameCount = static_cast<int>(RenderElements::turntableFrames.size());
	frame = std::clamp(frame, 0, frameCount - 1);

	return RenderElements::turntableFrames[frame];
}

/// @brief passes through a pre-baked static multi-line image (eg a frame border or decorative
/// divider) so callers draw it the same way as every other renderX element
/// @param std::string image the baked image to render
std::string Renderer::renderImage(const std::string& image)
{
	return image;
}

/// @brief renders a double, zero-padded at the start and truncated after the decimal point
/// to fit exactly promiseLength chars (drops the decimal point entirely if no room for any
/// fractional digits)
/// @param double value the value to render
/// @param int promiseLength the exact length the returned string should be
std::string Renderer::renderValue(double value, int promiseLength)
{
	std::string sign = value < 0 ? "-" : "";

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(10) << std::abs(value);
	std::string formatted = oss.str();

	size_t dotPos = formatted.find('.');
	std::string intDigits = formatted.substr(0, dotPos);
	std::string fracDigits = formatted.substr(dotPos + 1);

	int available = promiseLength - (int)sign.length();

	// truncate the fractional digits so intDigits + '.' + fracDigits fits
	int maxFrac = std::max(0, available - (int)intDigits.length() - 1);
	if ((int)fracDigits.length() > maxFrac)
		fracDigits.resize(maxFrac);

	std::string digits = fracDigits.empty() ? intDigits : (intDigits + "." + fracDigits);

	// pad the start with 0s to reach promiseLength
	int currentLength = (int)(sign.length() + digits.length());
	if (currentLength < promiseLength)
		digits = std::string(promiseLength - currentLength, '0') + digits;

	return sign + digits;
}

/// @brief renders a float, see the double overload for behavior
/// @param float value the value to render
/// @param int promiseLength the exact length the returned string should be
std::string Renderer::renderValue(float value, int promiseLength)
{
	return renderValue((double)value, promiseLength);
}

/// @brief renders a sliding, looping window of a string
/// @param std::string value the text to slide the window over
/// @param int promiseLength the exact length (window size) of the returned string
/// @param int windowPos how far into the looped text (text + one space, repeating) the
/// window starts; any int works, negative or larger than the loop included
std::string Renderer::renderValue(const std::string& value, int promiseLength, int windowPos)
{
	if (value.empty() || promiseLength <= 0)
		return std::string(std::max(0, promiseLength), ' ');

	std::string loopUnit = value + " ";
	int period = (int)loopUnit.length();

	// normalize windowPos into [0, period) so any int, including negative, works
	int startPos = ((windowPos % period) + period) % period;

	std::string result;
	result.reserve(promiseLength);

	for (int i = 0; i < promiseLength; i++)
		result += loopUnit[(startPos + i) % period];

	return result;
}

/// @brief renders an int, zero-padded at the start; if it doesn't fit promiseLength,
/// falls back to the string overload (at windowPos) to scroll it instead of truncating digits
/// @param int value the value to render
/// @param int promiseLength the exact length the returned string should be
/// @param int windowPos only used if value doesn't fit promiseLength, see the string overload
std::string Renderer::renderValue(int value, int promiseLength, int windowPos)
{
	std::string sign = value < 0 ? "-" : "";
	std::string digits = std::to_string(std::abs(value));

	int available = promiseLength - (int)sign.length();

	if ((int)digits.length() > available)
		return renderValue(sign + digits, promiseLength, windowPos);

	if ((int)digits.length() < available)
		digits = std::string(available - digits.length(), '0') + digits;

	return sign + digits;
}

/// @brief replaces every run of spaces with an ANSI cursor-forward escape of the same
/// width, so printing the result over existing terminal content leaves whatever was under
/// those spaces untouched instead of overwriting it with blanks
/// @param std::string image the string to convert
std::string Renderer::makeTransparent(std::string image)
{
	std::string result;
	result.reserve(image.length());

	size_t i = 0;
	while (i < image.length())
	{
		if (image[i] == ' ')
		{
			size_t runStart = i;
			while (i < image.length() && image[i] == ' ')
				i++;

			result += "\033[" + std::to_string(i - runStart) + "C";
		}
		else
		{
			result += image[i];
			i++;
		}
	}

	return result;
}

/// @brief renders one spectrum row, mapping each dominant frequency range index to its
/// RenderElements::spectrumChars entry
/// @param std::vector<int> dominantFreqRanges indices into RenderElements::spectrumChars /
/// RenderElements::frequencyRanges; -1 (or any other out-of-range index) renders as
/// RenderElements::spectrumBlank
std::string Renderer::renderSpectrum(const std::vector<int>& dominantFreqRanges)
{
	std::string result;
	result.reserve(dominantFreqRanges.size());

	int charCount = (int)RenderElements::spectrumChars.size();

	for (int freqRange : dominantFreqRanges)
	{
		if (freqRange < 0 || freqRange >= charCount)
			result += RenderElements::spectrumBlank;
		else
			result += RenderElements::spectrumChars[freqRange];
	}

	return result;
}

/// @brief renders a waveform window as a row of spectrum chars, one per charCount slice: averages
/// each slice's absolute amplitude, normalizes against the loudest slice in the window, and maps
/// that to a RenderElements::spectrumChars entry (index 0 = loudest, last = silent)
/// @param std::vector<float> waveform the audio window to analyze (eg Deck::extractExpectedWaveform)
/// @param int sampleRate the waveform's sample rate, only used to reject an unloaded deck here
/// @param int charCount how many time-slices (chars) to produce across the window
std::string Renderer::renderWaveform(const std::vector<float>& waveform, int sampleRate, int charCount)
{
	// an unloaded deck reports an empty waveform; bail before hopSize's divide and before
	// indexing into a buffer that isn't there
	if (waveform.empty() || sampleRate <= 0 || charCount <= 0)
		return std::string(std::max(0, charCount), RenderElements::spectrumBlank);

	// how many samples each char averages over; wider than hopSize on purpose, so
	// neighbouring slices overlap and the trace reads smoother than one slice per char would
	constexpr size_t averagingWindow = 10000;

	std::string result = "";
	size_t hopSize = waveform.size() / (size_t)charCount;

	std::vector<float> waveformVolumes(charCount, 0.0f);

	for (int currentChar = 0; currentChar < charCount; currentChar++)
	{
		// computed directly from currentChar (not accumulated hop-by-hop), so slice 0 starts
		// exactly at sample 0 and the array's true center - the playhead, per
		// Deck::extractExpectedWaveform's centered window - always lands exactly on the
		// boundary between the two middle slices, regardless of charCount/hopSize rounding
		size_t currentSample = (size_t)currentChar * hopSize;

		// averagingWindow is wider than hopSize, so the last slices would otherwise run off
		// the end of the buffer and average in whatever the allocator parked there (in
		// practice the *other* deck's waveform, which is allocated right after this one in
		// TUIdisplay::gatherTUItelemetry) - clamp to the buffer and divide by what was
		// actually summed so the tail slices stay this deck's own audio
		size_t sliceEnd = std::min(waveform.size(), currentSample + averagingWindow);
		size_t sliceLength = sliceEnd - currentSample;

		float waveformVolume = 0.0f;
		for (size_t x = currentSample; x < sliceEnd; x++)
		{
			waveformVolume += std::abs(waveform[x]);
		}

		waveformVolumes[currentChar] = sliceLength > 0 ? waveformVolume / (float)sliceLength : 0.0f;
	}

	// normalize against the loudest slice in this window, so the loudest char is always 1.0
	float maxVolume = 0.0f;
	for (float volume : waveformVolumes)
		maxVolume = std::max(maxVolume, volume);

	for (int currentChar = 0; currentChar < charCount; currentChar++)
	{
		float normalizedVolume = maxVolume > 0.0f ? waveformVolumes[currentChar] / maxVolume : 0.0f;

		int spectrumVolume = std::clamp((int)RenderElements::spectrumChars.size() - (int)(normalizedVolume * RenderElements::spectrumChars.size()), 0, (int)RenderElements::spectrumChars.size() - 1);

		result += RenderElements::spectrumChars[spectrumVolume];
	}

	return result;

	constexpr int binCount = 512;

	int fftSize = 1;
	while (fftSize < binCount * 2)
		fftSize *= 2;

	std::vector<std::vector<float>> stft = WaveformMath::FFTprocessor::computeSTFT(waveform, charCount, binCount);
	int bandCount = (int)RenderElements::frequencyRanges.size();

	std::vector<int> dominantFreqRanges(stft.size(), -1);

	for (size_t frame = 0; frame < stft.size(); frame++)
	{
		std::vector<float> bandEnergy(bandCount, 0.0f);

		for (int bin = 0; bin < binCount; bin++)
		{
			float freq = (float)bin * (float)sampleRate / (float)fftSize;

			for (int band = 0; band < bandCount; band++)
			{
				if (freq >= RenderElements::frequencyRanges[band].first && freq < RenderElements::frequencyRanges[band].second)
				{
					bandEnergy[band] += stft[frame][bin];
					break;
				}
			}
		}

		int bestBand = -1;
		float bestEnergy = 0.0f;

		for (int band = 0; band < bandCount; band++)
		{
			if (bandEnergy[band] > bestEnergy)
			{
				bestEnergy = bandEnergy[band];
				bestBand = band;
			}
		}

		dominantFreqRanges[frame] = bestBand;
	}

	return renderSpectrum(dominantFreqRanges);
}

}
