#pragma once

#include <string>
#include <vector>
#include <ranges>
#include <RendererStyle.hpp>
#include <FFTprocessor.hpp>

namespace Graphics
{

class Renderer
{
private:

public:
	static std::string renderHorizontalSlider(int length, int progress);
	static std::string renderVerticalSlider(int length, int progress);
	static std::string renderHorizontalFill(int length, int progress);
	static std::string renderVerticalFill(int length, int progress);
	static std::string renderKnob(float position);
	static std::string renderTurntable(int frame);

	static std::string renderValue(float value, int promiseLength);
	static std::string renderValue(double value, int promiseLength);
	static std::string renderValue(const std::string& value, int promiseLength, int windowPos);
	static std::string renderValue(int value, int promiseLength, int windowPos);

	static std::string makeTransparent(std::string image);

	static std::string renderSpectrum(const std::vector<int>& dominantFreqRanges);
};

}
