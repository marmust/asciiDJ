#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <DirReader.hpp>

namespace Graphics
{

// seconds per turntable frame at |speed + shift| == 1.0, scales down as the rate grows
inline constexpr double turntableBaseInterval = 0.14;

// |speed + shift| at or below this counts as stopped
inline constexpr double turntableStopThreshold = 0.01;

// chars per second a scrolling label's window position advances
inline constexpr double marqueeScrollRate = 4.0;

// waveform trace width and start column; width is odd so the playhead gets its own center char
inline constexpr int waveformCharCount = 89;
inline constexpr int waveformOriginX = 13;

// column the playhead marker draws at: the waveform's center char
inline constexpr int playheadMarkerX = waveformOriginX + waveformCharCount / 2;

// char length of the deck volume sliders and the crossfader slider
inline constexpr int volumeSliderLength = 13;
inline constexpr int xfaderSliderLength = 13;

// file-select list: rows per page, max chars per filename, blank rows between entries
inline constexpr int fileListPageRows = 8;
inline constexpr int fileListNameLength = 40;
inline constexpr int fileListSpacing = 1;

// all telemetry needed to feed TUI
struct TUItelemetry
{
	// deck1
	double d1speed = 1.0;
	double d1shift = 0.0;
	std::vector<float> d1waveform;
	float d1waveformReference = 0.0f;
	std::string d1name;
	float d1progress = 0.0f;
	double d1timeRemaining = 0.0;

	// deck2
	double d2speed = 1.0;
	double d2shift = 0.0;
	std::vector<float> d2waveform;
	float d2waveformReference = 0.0f;
	std::string d2name;
	float d2progress = 0.0f;
	double d2timeRemaining = 0.0;

	// mixer
	double d1volume = 1.0;
	double d2volume = 1.0;
	double xfader = 0.0;

	double deck1EQhigh = 0.0;
	double deck1EQmid = 0.0;
	double deck1EQlow = 0.0;

	double deck2EQhigh = 0.0;
	double deck2EQmid = 0.0;
	double deck2EQlow = 0.0;

	// file browser - populated fresh by TUIdisplay::gatherTUItelemetry() every tick
	std::vector<Files::FileEntry> fileList;
	std::string localDir;
	bool fileSelectOpen = false;
	int fileSelectedIdx = 0;
};

class GraphicsComposer
{
private:
	TUItelemetry reportedTelemetry;

	std::vector<std::string> frameBuffer;

	// turntable animation state per deck: current frame and when it started showing
	int turntable1Frame = 0;
	std::chrono::steady_clock::time_point turntable1LastTick = std::chrono::steady_clock::now();

	int turntable2Frame = 0;
	std::chrono::steady_clock::time_point turntable2LastTick = std::chrono::steady_clock::now();

	// wall-clock scroll state for each label rendered with Renderer::renderValue's marquee overload
	int deck1LabelWindowPos = 0;
	std::chrono::steady_clock::time_point deck1LabelLastTick = std::chrono::steady_clock::now();

	int deck2LabelWindowPos = 0;
	std::chrono::steady_clock::time_point deck2LabelLastTick = std::chrono::steady_clock::now();

	// waveform normalizer state per deck: trailing average of the loudest visible slice, last update
	float waveform1Normalizer = 0.0f;
	std::chrono::steady_clock::time_point waveform1LastTick = std::chrono::steady_clock::now();

	float waveform2Normalizer = 0.0f;
	std::chrono::steady_clock::time_point waveform2LastTick = std::chrono::steady_clock::now();

	int advanceTurntableFrame(double speed, double shift, int& frameState,
				   std::chrono::steady_clock::time_point& lastTick);

	int advanceDisplayValue(int& windowPos, std::chrono::steady_clock::time_point& lastTick);

	float advanceWaveformNormalizer(float windowMax, float& normalizer,
					std::chrono::steady_clock::time_point& lastTick);

	std::string composeWaveform(const std::vector<float>& waveform, float& normalizer,
				    std::chrono::steady_clock::time_point& lastTick);

	// text formatting shared by the speed/xfader readouts and the EQ dB readouts
	static std::string formatSignedReading(const std::string& prefix, const std::string& infix, double value);
	static std::string formatSignedDb(double value);

	// maps a control's value onto [0, 1] across its own min/max, for sliders and knobs
	static double rangeFraction(double value, double min, double max);

public:
	// ctor / dtor
	GraphicsComposer();
	~GraphicsComposer() = default;

	// telemetry setter
	void reportAudioEngineTelemetry(TUItelemetry telemetry);

	// meat and potatoes
	void composeFrame();

	// composes the file browser frame, used while file-select mode is on
	void composeFileSelectFrame();

	const std::vector<std::string>* getFrameBuffer() const;

	void drawAtPosition(std::string& image, int x, int y);
};

}
