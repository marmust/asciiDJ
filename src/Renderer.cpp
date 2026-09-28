#include <Renderer.hpp>
#include <WaveformTuning.hpp>

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

/// @brief renders a vertical selector string with a pointer marking one stop out of length,
/// very close to renderVerticalSlider, but instead of the pointer landing on any of length
/// individual lines, it only lands on one of length+1 evenly spaced stops - spacing filler
/// lines apart (spacing 0 = a stop every line, spacing 1 = every other line, etc)
/// @param int length how many stops (0..length) the selector has
/// @param int value which stop, out of [0, length], to put the pointer on
/// @param int spacing how many filler lines separate each stop
std::string Renderer::renderVerticalSelector(int length, int value, int spacing)
{
	int stride = spacing + 1;

	auto preFill = std::views::repeat(RenderElements::verticalSelectorBackground, value * stride) | std::views::join;
	auto postFill = std::views::repeat(RenderElements::verticalSelectorBackground, (length - value) * stride) | std::views::join;

	std::string result = std::string(preFill.begin(), preFill.end()) +
			      RenderElements::verticalSelectorPointer +
			      std::string(postFill.begin(), postFill.end());

	// every line (background and pointer alike) bakes in its own trailing "\n" so the repeats
	// above join cleanly - drop the one dangling off the very last line to match every other
	// renderer here, none of which leave a trailing newline
	result.pop_back();

	return result;
}

/// @brief renders a paginated file list: truncationHeight rows, one file name per row at its own
/// natural length (via renderValue's string overload, promiseLength set to the name's own
/// length so it renders as-is rather than looping into repeats), blank (truncationLength-wide)
/// rows past the end of files. selected picks which page (rows
/// [selected/truncationHeight*truncationHeight, +truncationHeight)) is shown - the caller is
/// expected to separately drive a renderVerticalSelector(truncationHeight - 1, selected %
/// truncationHeight, spacing) alongside this, and passing the same spacing here keeps that
/// selector's pointer lined up with the currently selected row
/// @param std::vector<Files::FileEntry> files the full file listing to page through
/// @param int truncationLength width of the blank rows past the end of files (no longer clamps
/// file name length - a name longer than this now renders in full rather than being truncated)
/// @param int truncationHeight how many rows are visible at once (the page size)
/// @param int selected which file, out of the full files vec, is currently selected
/// @param int spacing how many filler lines separate each row
std::string Renderer::renderFileList(const std::vector<Files::FileEntry>& files, int truncationLength, int truncationHeight, int selected, int spacing)
{
	if (truncationHeight <= 0)
		return "";

	int pageStart = (selected / truncationHeight) * truncationHeight;
	std::string blankRow(std::max(0, truncationLength), ' ');

	std::string result;

	for (int row = 0; row < truncationHeight; row++)
	{
		int fileIdx = pageStart + row;

		// promiseLength set to the name's own length (not truncationLength) so renderValue
		// doesn't loop it into repeats - its behavior whenever promiseLength exceeds the text
		result += (fileIdx >= 0 && fileIdx < (int)files.size())
				  ? renderValue(files[fileIdx].displayName, (int)files[fileIdx].displayName.length(), 0)
				  : blankRow;

		// no filler (or newline) trailing the very last row, matching every other renderer's
		// "no dangling trailing newline" convention
		if (row < truncationHeight - 1)
		{
			result += "\n";

			for (int fillerLine = 0; fillerLine < spacing; fillerLine++)
				result += blankRow + "\n";
		}
	}

	return result;
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

/// @brief measures a waveform window's loudness per char: each of charCount slices gets the mean
/// absolute amplitude over a Waveform::AVERAGING_FRAMES window centered on it
/// @param std::vector<float> waveform the audio window to analyze (eg Deck::extractExpectedWaveform)
/// @param int charCount how many time-slices (chars) to measure across the window
/// @returns std::vector<float> one volume per slice, empty if there's no waveform (no track loaded)
std::vector<float> Renderer::measureWaveform(const std::vector<float>& waveform, int charCount)
{
	// an unloaded deck reports an empty waveform; bail before hopSize's divide and before
	// indexing into a buffer that isn't there
	if (waveform.empty() || charCount <= 0)
		return {};

	const size_t halfAveraging = Waveform::AVERAGING_FRAMES / 2;
	const size_t hopSize = waveform.size() / (size_t)charCount;

	std::vector<float> sliceVolumes((size_t)charCount, 0.0f);

	for (int currentChar = 0; currentChar < charCount; currentChar++)
	{
		// computed directly from currentChar (not accumulated hop-by-hop), so slice 0 starts
		// exactly at sample 0 and the array's true center - the playhead, per
		// Deck::extractExpectedWaveform's centered window - always lands exactly on the
		// boundary between the two middle slices, regardless of charCount/hopSize rounding
		size_t sliceCenter = (size_t)currentChar * hopSize + hopSize / 2;

		// centered on the slice (not starting at it), so a transient lights up the chars around
		// where it actually is rather than the ones before it. clamped to the buffer at both ends
		// and divided by what was actually summed, so edge slices never read past this deck's audio
		size_t sliceStart = sliceCenter > halfAveraging ? sliceCenter - halfAveraging : 0;
		size_t sliceEnd = std::min(waveform.size(), sliceCenter + halfAveraging);

		float amplitudeSum = 0.0f;
		for (size_t x = sliceStart; x < sliceEnd; x++)
			amplitudeSum += std::abs(waveform[x]);

		sliceVolumes[currentChar] = sliceEnd > sliceStart ? amplitudeSum / (float)(sliceEnd - sliceStart) : 0.0f;
	}

	return sliceVolumes;
}

/// @brief renders measured slice volumes as a row of RenderElements::waveformChars (index 0 =
/// loudest), each scaled against normalizer and clipped to full scale
/// @param std::vector<float> sliceVolumes per-char volumes (eg from measureWaveform)
/// @param float normalizer the volume that renders as full scale; <= 0 renders blank
/// @param int charCount how many chars to produce; blank if sliceVolumes doesn't match it
std::string Renderer::renderWaveform(const std::vector<float>& sliceVolumes, float normalizer, int charCount)
{
	if (charCount <= 0 || normalizer <= 0.0f || (int)sliceVolumes.size() != charCount)
		return std::string(std::max(0, charCount), RenderElements::waveformBlank);

	const int levelCount = (int)RenderElements::waveformChars.size();

	std::string result;
	result.reserve(charCount);

	for (float sliceVolume : sliceVolumes)
	{
		// the normalizer can trail below a freshly loud window, so clip rather than overflow
		float normalizedVolume = std::min(1.0f, sliceVolume / normalizer);

		int level = std::clamp(levelCount - (int)(normalizedVolume * levelCount), 0, levelCount - 1);
		result += RenderElements::waveformChars[level];
	}

	return result;
}
}
