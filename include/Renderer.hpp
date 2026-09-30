#pragma once

#include <string>
#include <vector>
#include <DirReader.hpp>

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
	static std::string renderVerticalSelector(int length, int value, int spacing);
	static std::string renderFileList(const std::vector<Files::FileEntry>& files, int truncationLength, int truncationHeight, int selected, int spacing);
	static std::string renderKnob(float position);
	static std::string renderTurntable(int frame);
	static std::string renderImage(const std::string& image);

	static std::string renderValue(float value, int promiseLength);
	static std::string renderValue(double value, int promiseLength);
	static std::string renderValue(const std::string& value, int promiseLength, int windowPos);
	static std::string renderValue(int value, int promiseLength, int windowPos);

	static std::string makeTransparent(std::string image);

	static std::vector<float> measureWaveform(const std::vector<float>& waveform, int charCount);
	static std::string renderWaveform(const std::vector<float>& sliceVolumes, float normalizer, int charCount);
};

}
