#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace Graphics
{

// seconds per turntable frame at |speed + shift| == 1.0; interval scales down as
// |speed + shift| grows, so a faster/harder-scratched deck visibly spins faster
inline constexpr double turntableBaseInterval = 0.14;

// |speed + shift| at or below this is clamped to an exact stop, so a paused deck doesn't
// slowly creep from leftover rate instead of comparing floats to 0 directly
inline constexpr double turntableStopThreshold = 0.01;

// chars per second a scrolling label's window position advances
inline constexpr double marqueeScrollRate = 4.0;

// how many chars wide each deck's waveform trace renders, and which column it starts at.
// odd, so the window's center - the playhead - gets a char of its own to sit under rather
// than falling between two
inline constexpr int waveformCharCount = 89;
inline constexpr int waveformOriginX = 13;

// column the playhead marker draws at: the waveform's center char
inline constexpr int playheadMarkerX = waveformOriginX + waveformCharCount / 2;

// all telemetry needed to feed TUI
struct TUItelemetry
{
	// deck1
	double d1speed = 1.0;
	double d1shift = 0.0;
	std::vector<float> d1waveform;
	int d1sampleRate = 0;
	std::string d1name;
	float d1progress = 0.0f;
	double d1timeRemaining = 0.0;

	// deck2
	double d2speed = 1.0;
	double d2shift = 0.0;
	std::vector<float> d2waveform;
	int d2sampleRate = 0;
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
};

class GraphicsComposer
{
private:
	TUItelemetry reportedTelemetry;

	std::vector<std::string> frameBuffer;

	// wall-clock turntable animation state, one set per deck: which frame is currently
	// showing, and when it started showing - so frame advancement is driven by real elapsed
	// time rather than by how often composeFrame() happens to get called
	int turntable1Frame = 0;
	std::chrono::steady_clock::time_point turntable1LastTick = std::chrono::steady_clock::now();

	int turntable2Frame = 0;
	std::chrono::steady_clock::time_point turntable2LastTick = std::chrono::steady_clock::now();

	// wall-clock scroll state for each label rendered with Renderer::renderValue's marquee overload
	int deck1LabelWindowPos = 0;
	std::chrono::steady_clock::time_point deck1LabelLastTick = std::chrono::steady_clock::now();

	int deck2LabelWindowPos = 0;
	std::chrono::steady_clock::time_point deck2LabelLastTick = std::chrono::steady_clock::now();

	int advanceTurntableFrame(double speed, double shift, int& frameState,
				   std::chrono::steady_clock::time_point& lastTick);

	int advanceDisplayValue(int& windowPos, std::chrono::steady_clock::time_point& lastTick);

	// text formatting shared by the speed/xfader readouts and the EQ dB readouts
	static std::string formatSignedReading(const std::string& prefix, const std::string& infix, double value);
	static std::string formatSignedDb(double value);

public:
	// ctor / dtor
	GraphicsComposer();
	~GraphicsComposer() = default;

	// telemetry setter
	void reportAudioEngineTelemetry(TUItelemetry telemetry);

	// meat and potatoes
	void composeFrame();

	const std::vector<std::string>* getFrameBuffer() const;

	void drawAtPosition(std::string& image, int x, int y);
};

}
