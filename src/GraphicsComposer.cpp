#include <GraphicsComposer.hpp>

#include <Renderer.hpp>
#include <RendererStyle.hpp>

#include <cmath>
#include <algorithm>

namespace Graphics
{

namespace
{
	// seconds per turntable frame at |speed + shift| == 1.0; interval scales down as
	// |speed + shift| grows, so a faster/harder-scratched deck visibly spins faster
	constexpr double turntableBaseInterval = 0.07;
}

/// @brief ctor
GraphicsComposer::GraphicsComposer() {}

/// @brief telemetry setter
/// @param TUItelemetry telemetry latest snapshot reported by the audio engine
void GraphicsComposer::reportAudioEngineTelemetry(TUItelemetry telemetry)
{
	this->reportedTelemetry = telemetry;
}

/// @brief shifts a string in a way where its top left most (first) char is y newlines down,
/// and x spaces to the right
/// @param std::string image the string to be shifted
/// @param int x the left shift (tabs) to draw at
/// @param int y the top shift (newlines) to draw at
/// @returns std::string the image string shifted by x and y
std::string GraphicsComposer::drawAtPosition(std::string image, int x, int y)
{
	std::string cursorForward = "\033[" + std::to_string(x) + "C";

	int pos = 0;

	while ((pos = image.find('\n', pos)) != std::string::npos)
	{
		image.replace(pos, 1, "\n" + cursorForward);
		pos += 1 + cursorForward.length();
	}

	return std::string(y, '\n') + cursorForward + image;
}

/// @brief composes a single TUI frame from the last reported telemetry
/// @return std::string the frame to print
std::string GraphicsComposer::composeFrame()
{
	int frameCount = static_cast<int>(RenderElements::turntableFrames.size());

	// deck1's rate: speed carries the base spin, shift layers a scratch on top; a negative
	// rate spins the turntable backwards, same as scratching a real record against playback
	double rate = this->reportedTelemetry.d1speed + this->reportedTelemetry.d1shift;
	double interval = turntableBaseInterval / std::max(std::abs(rate), 0.01);
	int direction = rate < 0.0 ? -1 : 1;

	// purely wall-time driven: however long it's actually been (regardless of how often or
	// rarely composeFrame() gets called) is divided by the interval to get whole frame steps,
	// and lastTickTime only advances by that many intervals so fractional time isn't dropped
	auto now = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed = now - this->lastTickTime;

	int steps = static_cast<int>(elapsed.count() / interval);
	if (steps > 0)
	{
		this->turntableFrame = ((this->turntableFrame + direction * steps) % frameCount + frameCount) % frameCount;
		this->lastTickTime += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
			std::chrono::duration<double>(steps * interval));
	}

	std::string turntable = Graphics::Renderer::renderTurntable(this->turntableFrame);

	return this->drawAtPosition(turntable, 0, 0);
}

}
