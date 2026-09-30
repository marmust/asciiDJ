#include <GraphicsComposer.hpp>
#include <WaveformTuning.hpp>
#include <InputTuning.hpp>

#include <Renderer.hpp>
#include <RendererStyle.hpp>

#include <cmath>
#include <algorithm>

namespace Graphics
{

/// @brief ctor
GraphicsComposer::GraphicsComposer() {}

/// @brief telemetry setter
/// @param TUItelemetry telemetry latest snapshot reported by the audio engine
void GraphicsComposer::reportAudioEngineTelemetry(TUItelemetry telemetry)
{
	this->reportedTelemetry = telemetry;
}

/// @brief shifts a string in place so its first char lands y rows down and x columns right
/// @param std::string& image the string to be shifted (modified in place)
/// @param int x the left shift (tabs) to draw at
/// @param int y the top shift (newlines) to draw at
void GraphicsComposer::drawAtPosition(std::string& image, int x, int y)
{
	std::string cursorForward = "\033[" + std::to_string(x) + "C";

	size_t pos = 0;

	while ((pos = image.find('\n', pos)) != std::string::npos)
	{
		image.replace(pos, 1, "\n" + cursorForward);
		pos += 1 + cursorForward.length();
	}

	image = std::string(y, '\n') + cursorForward + image;
}

/// @brief advances a turntable's spin frame by wall-clock time elapsed since the last advance
/// @param double speed the deck's playback speed
/// @param double shift the deck's turntable shift (scratch)
/// @param int& frameState this deck's persisted frame index, advanced in place
/// @param std::chrono::steady_clock::time_point& lastTick this deck's persisted last-tick time
/// @returns int the frame index to render this call
int GraphicsComposer::advanceTurntableFrame(double speed, double shift, int& frameState,
					     std::chrono::steady_clock::time_point& lastTick)
{
	int frameCount = static_cast<int>(RenderElements::turntableFrames.size());
	auto now = std::chrono::steady_clock::now();

	// a negative rate spins the turntable backwards, same as scratching a record against playback
	double rate = speed + shift;
	if (std::abs(rate) <= turntableStopThreshold)
	{
		lastTick = now;
		return frameState;
	}

	double interval = turntableBaseInterval / std::abs(rate);
	int direction = rate < 0.0 ? -1 : 1;

	std::chrono::duration<double> elapsed = now - lastTick;

	int steps = static_cast<int>(elapsed.count() / interval);
	if (steps > 0)
	{
		frameState = ((frameState + direction * steps) % frameCount + frameCount) % frameCount;
		lastTick += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
			std::chrono::duration<double>(steps * interval));
	}

	return frameState;
}

/// @brief eases a deck's waveform normalizer toward this frame's window max (NORMALIZER_HALFLIFE)
/// @param float windowMax the loudest slice volume currently in view
/// @param float& normalizer this deck's persisted normalizer, advanced in place
/// @param std::chrono::steady_clock::time_point& lastTick this deck's persisted last-update time
/// @returns float the normalizer to render this frame with
float GraphicsComposer::advanceWaveformNormalizer(float windowMax, float& normalizer,
						  std::chrono::steady_clock::time_point& lastTick)
{
	auto now = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed = now - lastTick;
	lastTick = now;

	// nothing to trail from yet, start at the real max
	if (normalizer <= 0.0f)
	{
		normalizer = windowMax;
		return normalizer;
	}

	double weight = 1.0 - std::pow(0.5, elapsed.count() / Waveform::NORMALIZER_HALFLIFE);
	normalizer += (float)weight * (windowMax - normalizer);

	return normalizer;
}

/// @brief measures a deck's waveform window, advances its normalizer and renders the trace
/// @param std::vector<float> waveform the deck's audio window (TUItelemetry::d1waveform/d2waveform)
/// @param float& normalizer this deck's persisted normalizer
/// @param std::chrono::steady_clock::time_point& lastTick this deck's persisted normalizer tick
/// @returns std::string the rendered trace, waveformCharCount chars wide
std::string GraphicsComposer::composeWaveform(const std::vector<float>& waveform, float& normalizer,
					      std::chrono::steady_clock::time_point& lastTick)
{
	std::vector<float> sliceVolumes = Renderer::measureWaveform(waveform, waveformCharCount);

	// unloaded deck: drop the trailing value so the next track seeds fresh from its own level
	if (sliceVolumes.empty())
	{
		normalizer = 0.0f;
		return Renderer::renderWaveform(sliceVolumes, normalizer, waveformCharCount);
	}

	float windowMax = *std::max_element(sliceVolumes.begin(), sliceVolumes.end());
	float trailingNormalizer = this->advanceWaveformNormalizer(windowMax, normalizer, lastTick);

	float frameNormalizer = trailingNormalizer * Waveform::NORMALIZER_SCALE;

	return Renderer::renderWaveform(sliceVolumes, frameNormalizer, waveformCharCount);
}

/// @brief advances a scrolling label's window position by wall-clock time at marqueeScrollRate
/// @param int& windowPos this label's persisted scroll position, advanced in place
/// @param std::chrono::steady_clock::time_point& lastTick this label's persisted last-tick time
/// @returns int the window position to render this call
int GraphicsComposer::advanceDisplayValue(int& windowPos, std::chrono::steady_clock::time_point& lastTick)
{
	auto now = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed = now - lastTick;

	int step = static_cast<int>(marqueeScrollRate * elapsed.count());
	if (step <= 0)
		return windowPos;

	lastTick = now;
	windowPos += step;

	return windowPos;
}

/// @brief formats a fixed-width signed reading, eg "speed  x1.0" / "speed x-1.0"
/// @param std::string prefix the leading label ("speed", "xfader")
/// @param std::string infix text right before the number, no sign ("x" for speed, "" for xfader)
/// @param double value the value to render, magnitude assumed <= 9.9
std::string GraphicsComposer::formatSignedReading(const std::string& prefix, const std::string& infix, double value)
{
	std::string sign = value < 0.0 ? "-" : "";
	std::string numText = Renderer::renderValue(std::abs(value), 3);
	int spaces = sign.empty() ? 2 : 1;

	return prefix + std::string(spaces, ' ') + infix + sign + numText;
}

/// @brief formats a signed dB reading with one decimal, zero-padded to fit the full EQ range
/// @param double value the dB value to render
std::string GraphicsComposer::formatSignedDb(double value)
{
	std::string sign = value < 0.0 ? "-" : "+";

	// digits in the largest magnitude the EQ can reach
	int maxMagnitude = (int)std::ceil(std::max(std::abs(Input::EQ_MIN), std::abs(Input::EQ_MAX)));
	int intDigits = (int)std::to_string(maxMagnitude).length();

	double absRounded = std::round(std::abs(value) * 10.0) / 10.0;
	int intPart = (int)absRounded;
	int fracPart = (int)std::lround((absRounded - intPart) * 10.0);

	if (fracPart >= 10)
	{
		fracPart -= 10;
		intPart += 1;
	}

	return sign + Renderer::renderValue(intPart, intDigits, 0) + "." + std::to_string(fracPart);
}

/// @brief maps a control's value onto [0, 1] across its own range, clamped
/// @param double value the control's current value
/// @param double min the control's minimum (maps to 0)
/// @param double max the control's maximum (maps to 1)
/// @returns double the fraction of the range value sits at, 0 for an empty range
double GraphicsComposer::rangeFraction(double value, double min, double max)
{
	if (max <= min)
		return 0.0;

	return std::clamp((value - min) / (max - min), 0.0, 1.0);
}

/// @brief composes a single TUI frame from the last reported telemetry into frameBuffer vec
void GraphicsComposer::composeFrame()
{
	const TUItelemetry& telemetry = this->reportedTelemetry;

	// static background: outer frame/header, and the EQ section's divider column
	std::string frame = Renderer::renderImage(RenderElements::frameImage);
	std::string divider = Renderer::renderImage(RenderElements::middleDividerImage);

	// per-slice amplitude row for each deck, centered on that deck's playhead
	std::string deck1Waveform = this->composeWaveform(telemetry.d1waveform, this->waveform1Normalizer, this->waveform1LastTick);
	std::string deck2Waveform = this->composeWaveform(telemetry.d2waveform, this->waveform2Normalizer, this->waveform2LastTick);

	// static "v" marking the playhead column both traces are centered on
	std::string playheadMarker = Renderer::renderImage(RenderElements::playheadMarkerImage);

	// spin each deck's turntable at its own wall-clock-driven rate
	int deck1Frame = this->advanceTurntableFrame(telemetry.d1speed, telemetry.d1shift,
						       this->turntable1Frame, this->turntable1LastTick);
	int deck2Frame = this->advanceTurntableFrame(telemetry.d2speed, telemetry.d2shift,
						       this->turntable2Frame, this->turntable2LastTick);

	std::string deck1 = Renderer::renderTurntable(deck1Frame);
	std::string deck2 = Renderer::renderTurntable(deck2Frame);

	// scroll each label's text window at its own wall-clock-driven rate
	int deck1LabelScroll = this->advanceDisplayValue(this->deck1LabelWindowPos, this->deck1LabelLastTick);
	int deck2LabelScroll = this->advanceDisplayValue(this->deck2LabelWindowPos, this->deck2LabelLastTick);

	// an unloaded deck's track name comes back empty
	std::string deck1label = Renderer::renderValue(telemetry.d1name.empty() ? "DECK 1 UNLOADED" : telemetry.d1name, 19, deck1LabelScroll);
	std::string deck2label = Renderer::renderValue(telemetry.d2name.empty() ? "DECK 2 UNLOADED" : telemetry.d2name, 19, deck2LabelScroll);

	// progress as a static zero-padded percentage (eg "093%")
	std::string deck1progress = Renderer::renderValue("DECK1: " + Renderer::renderValue((int)std::lround(telemetry.d1progress * 100.0f), 3, 0) + "%", 11, 0);
	std::string deck2progress = Renderer::renderValue("DECK2: " + Renderer::renderValue((int)std::lround(telemetry.d2progress * 100.0f), 3, 0) + "%", 11, 0);

	// knobs take [-1, 1]: -1 at Input::EQ_MIN, +1 at Input::EQ_MAX
	std::string EQ1highKnob = Renderer::renderKnob((float)(rangeFraction(telemetry.deck1EQhigh, Input::EQ_MIN, Input::EQ_MAX) * 2.0 - 1.0));
	std::string EQ1midKnob = Renderer::renderKnob((float)(rangeFraction(telemetry.deck1EQmid, Input::EQ_MIN, Input::EQ_MAX) * 2.0 - 1.0));
	std::string EQ1lowKnob = Renderer::renderKnob((float)(rangeFraction(telemetry.deck1EQlow, Input::EQ_MIN, Input::EQ_MAX) * 2.0 - 1.0));

	std::string EQ2highKnob = Renderer::renderKnob((float)(rangeFraction(telemetry.deck2EQhigh, Input::EQ_MIN, Input::EQ_MAX) * 2.0 - 1.0));
	std::string EQ2midKnob = Renderer::renderKnob((float)(rangeFraction(telemetry.deck2EQmid, Input::EQ_MIN, Input::EQ_MAX) * 2.0 - 1.0));
	std::string EQ2lowKnob = Renderer::renderKnob((float)(rangeFraction(telemetry.deck2EQlow, Input::EQ_MIN, Input::EQ_MAX) * 2.0 - 1.0));

	// dB readouts next to each knob, fixed width sized to the EQ range (see formatSignedDb)
	std::string EQ1highDb = this->formatSignedDb(telemetry.deck1EQhigh);
	std::string EQ1midDb = this->formatSignedDb(telemetry.deck1EQmid);
	std::string EQ1lowDb = this->formatSignedDb(telemetry.deck1EQlow);

	std::string EQ2highDb = this->formatSignedDb(telemetry.deck2EQhigh);
	std::string EQ2midDb = this->formatSignedDb(telemetry.deck2EQmid);
	std::string EQ2lowDb = this->formatSignedDb(telemetry.deck2EQlow);

	// slider progress climbs from the top as volume rises through Input::VOLUME_MIN..VOLUME_MAX
	double deck1VolumeFraction = rangeFraction(telemetry.d1volume, Input::VOLUME_MIN, Input::VOLUME_MAX);
	double deck2VolumeFraction = rangeFraction(telemetry.d2volume, Input::VOLUME_MIN, Input::VOLUME_MAX);
	int deck1VolumeProgress = (int)std::lround((1.0 - deck1VolumeFraction) * (volumeSliderLength - 1));
	int deck2VolumeProgress = (int)std::lround((1.0 - deck2VolumeFraction) * (volumeSliderLength - 1));

	std::string deck1volume = Renderer::renderVerticalSlider(volumeSliderLength, deck1VolumeProgress);
	std::string deck2volume = Renderer::renderVerticalSlider(volumeSliderLength, deck2VolumeProgress);

	// numeric part is never negative, always exactly 3 chars ("0.0" .. "1.0")
	std::string deck1volumeLabel = "vol " + Renderer::renderValue(telemetry.d1volume, 3);
	std::string deck2volumeLabel = "vol " + Renderer::renderValue(telemetry.d2volume, 3);

	// crossfader travels left to right across Input::XFADER_MIN..XFADER_MAX
	double xfaderFraction = rangeFraction(telemetry.xfader, Input::XFADER_MIN, Input::XFADER_MAX);
	int xfaderProgress = (int)std::lround(xfaderFraction * (xfaderSliderLength - 1));

	std::string crossfader = Renderer::renderHorizontalSlider(xfaderSliderLength, xfaderProgress);

	// the sign takes one of the two separator spaces, so the width never changes
	std::string deck1speedLabel = this->formatSignedReading("speed", "x", telemetry.d1speed);
	std::string deck2speedLabel = this->formatSignedReading("speed", "x", telemetry.d2speed);
	std::string xfaderLabel = this->formatSignedReading("xfader", "", telemetry.xfader);

	// content time left (ignores playback speed), clamped to 9:59 to keep the width fixed
	int deck1RemainingSecs = std::clamp((int)std::lround(telemetry.d1timeRemaining), 0, 599);
	int deck2RemainingSecs = std::clamp((int)std::lround(telemetry.d2timeRemaining), 0, 599);

	std::string deck1TimeRemaining = "time remaining " + std::to_string(deck1RemainingSecs / 60) + ":" + Renderer::renderValue(deck1RemainingSecs % 60, 2, 0);
	std::string deck2TimeRemaining = "time remaining " + std::to_string(deck2RemainingSecs / 60) + ":" + Renderer::renderValue(deck2RemainingSecs % 60, 2, 0);

	// every element pinned to a fixed grid cell, backgrounds first so dynamic elements draw over them
	this->drawAtPosition(frame, 0, 0);
	this->drawAtPosition(divider, 34, 5);

	// volume sliders flank the divider, one column outside it on each side
	this->drawAtPosition(deck1volume, 33, 5);
	this->drawAtPosition(deck2volume, 69, 5);

	// marker sits one row above both traces, which are stacked on rows 2 and 3
	this->drawAtPosition(playheadMarker, playheadMarkerX, 1);

	this->drawAtPosition(deck1Waveform, waveformOriginX, 2);
	this->drawAtPosition(deck2Waveform, waveformOriginX, 3);

	this->drawAtPosition(deck1, 3, 5);
	this->drawAtPosition(deck2, 73, 5);

	this->drawAtPosition(deck1label, 7, 8);
	this->drawAtPosition(deck2label, 77, 8);

	this->drawAtPosition(deck1progress, 11, 15);
	this->drawAtPosition(deck2progress, 81, 15);

	this->drawAtPosition(EQ1highKnob, 42, 5);
	this->drawAtPosition(EQ1midKnob, 42, 10);
	this->drawAtPosition(EQ1lowKnob, 42, 15);

	this->drawAtPosition(EQ2highKnob, 54, 5);
	this->drawAtPosition(EQ2midKnob, 54, 10);
	this->drawAtPosition(EQ2lowKnob, 54, 15);

	this->drawAtPosition(EQ1highDb, 36, 7);
	this->drawAtPosition(EQ1midDb, 36, 12);
	this->drawAtPosition(EQ1lowDb, 36, 17);

	this->drawAtPosition(EQ2highDb, 62, 7);
	this->drawAtPosition(EQ2midDb, 62, 12);
	this->drawAtPosition(EQ2lowDb, 62, 17);

	this->drawAtPosition(deck1speedLabel, 11, 20);
	this->drawAtPosition(deck2speedLabel, 81, 20);
	this->drawAtPosition(crossfader, 44, 20);

	this->drawAtPosition(deck1TimeRemaining, 3, 21);
	this->drawAtPosition(deck1volumeLabel, 30, 21);
	this->drawAtPosition(xfaderLabel, 46, 21);
	this->drawAtPosition(deck2volumeLabel, 66, 21);
	this->drawAtPosition(deck2TimeRemaining, 81, 21);

	// reset from the previous call, so this doesn't grow unbounded across repeated frames
	this->frameBuffer.clear();

	this->frameBuffer.push_back(frame);
	this->frameBuffer.push_back(divider);

	this->frameBuffer.push_back(deck1volume);
	this->frameBuffer.push_back(deck2volume);

	this->frameBuffer.push_back(playheadMarker);

	this->frameBuffer.push_back(deck1Waveform);
	this->frameBuffer.push_back(deck2Waveform);

	this->frameBuffer.push_back(deck1);
	this->frameBuffer.push_back(deck2);

	this->frameBuffer.push_back(deck1label);
	this->frameBuffer.push_back(deck2label);

	this->frameBuffer.push_back(deck1progress);
	this->frameBuffer.push_back(deck2progress);

	this->frameBuffer.push_back(EQ1highKnob);
	this->frameBuffer.push_back(EQ1midKnob);
	this->frameBuffer.push_back(EQ1lowKnob);

	this->frameBuffer.push_back(EQ2highKnob);
	this->frameBuffer.push_back(EQ2midKnob);
	this->frameBuffer.push_back(EQ2lowKnob);

	this->frameBuffer.push_back(EQ1highDb);
	this->frameBuffer.push_back(EQ1midDb);
	this->frameBuffer.push_back(EQ1lowDb);

	this->frameBuffer.push_back(EQ2highDb);
	this->frameBuffer.push_back(EQ2midDb);
	this->frameBuffer.push_back(EQ2lowDb);

	this->frameBuffer.push_back(deck1speedLabel);
	this->frameBuffer.push_back(deck2speedLabel);
	this->frameBuffer.push_back(crossfader);

	this->frameBuffer.push_back(deck1TimeRemaining);
	this->frameBuffer.push_back(deck1volumeLabel);
	this->frameBuffer.push_back(xfaderLabel);
	this->frameBuffer.push_back(deck2volumeLabel);
	this->frameBuffer.push_back(deck2TimeRemaining);
}

/// @brief composes the file-select frame: border, header, selector column and file list
void GraphicsComposer::composeFileSelectFrame()
{
	const TUItelemetry& telemetry = this->reportedTelemetry;

	// row highlighted within the visible page
	int pageLocalSelected = telemetry.fileSelectedIdx % fileListPageRows;

	bool hasSelection = telemetry.fileSelectedIdx >= 0 && telemetry.fileSelectedIdx < (int)telemetry.fileList.size();
	std::string selectedName = hasSelection ? telemetry.fileList[telemetry.fileSelectedIdx].displayName : "";

	std::string frame = Renderer::renderImage(RenderElements::frameImage);

	// waveform traces next to the frame's DECKn labels
	std::string deck1Waveform = this->composeWaveform(telemetry.d1waveform, this->waveform1Normalizer, this->waveform1LastTick);
	std::string deck2Waveform = this->composeWaveform(telemetry.d2waveform, this->waveform2Normalizer, this->waveform2LastTick);

	// promiseLength = each line's own length, so renderValue neither loops nor pads it
	std::string openDirLine = "open directory: " + telemetry.localDir;
	std::string selectedLine = "selected file: " + selectedName;

	std::string header = Renderer::renderValue(openDirLine, (int)openDirLine.length(), 0) + "\n" +
			      Renderer::renderValue(selectedLine, (int)selectedLine.length(), 0);

	std::string selectorCol = Renderer::renderVerticalSelector(fileListPageRows - 1, pageLocalSelected, fileListSpacing);
	std::string fileListCol = Renderer::renderFileList(telemetry.fileList, fileListNameLength, fileListPageRows, telemetry.fileSelectedIdx, fileListSpacing);

	// waveforms stay at waveformOriginX next to the frame's DECKn labels
	this->drawAtPosition(frame, 0, 0);
	this->drawAtPosition(deck1Waveform, waveformOriginX, 2);
	this->drawAtPosition(deck2Waveform, waveformOriginX, 3);
	this->drawAtPosition(header, 4, 5);
	this->drawAtPosition(selectorCol, 4, 8);
	this->drawAtPosition(fileListCol, 7, 8);

	this->frameBuffer.clear();

	this->frameBuffer.push_back(frame);
	this->frameBuffer.push_back(deck1Waveform);
	this->frameBuffer.push_back(deck2Waveform);
	this->frameBuffer.push_back(header);
	this->frameBuffer.push_back(selectorCol);
	this->frameBuffer.push_back(fileListCol);
}

/// @brief accessor for the elements composeFrame() last built, for callers (TUIdisplay) to print
/// @returns const std::vector<std::string>& the composed frame's elements
const std::vector<std::string>* GraphicsComposer::getFrameBuffer() const
{
	return &this->frameBuffer;
}

}
