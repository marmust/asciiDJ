#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace Graphics
{


// all telemetry needed to feed TUI
struct TUItelemetry
{
	// deck1
	double d1speed = 1.0;
	double d1shift = 0.0;
	std::vector<float> d1waveform;

	// deck2
	double d2speed = 1.0;
	double d2shift = 0.0;
	std::vector<float> d2waveform;

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

	// wall-clock turntable animation state: which frame is currently showing, and when it
	// started showing - so frame advancement is driven by real elapsed time rather than by
	// how often composeFrame() happens to get called
	int turntableFrame = 0;
	std::chrono::steady_clock::time_point lastTickTime = std::chrono::steady_clock::now();

	std::string drawAtPosition(std::string image, int x,  int y);

public:
	// ctor / dtor
	GraphicsComposer();
	~GraphicsComposer() = default;

	// telemetry setter
	void reportAudioEngineTelemetry(TUItelemetry telemetry);

	// meat and potatoes
	std::string composeFrame();
};

}
