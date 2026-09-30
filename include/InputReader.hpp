#pragma once

#include <linux/input.h>
#include <vector>

namespace Input
{

// reads system-wide key state via evdev; needs root or the input group (sudo usermod -aG input $USER)
class InputReader
{
private:
	// every keyboard device that could be opened, their key states are merged
	std::vector<int> keyboardFds;

	// snapshot of which keys are down, one bit per evdev key code, refreshed by poll()
	unsigned char keyState[(KEY_MAX + 7) / 8] = {0};

	void openKeyboards();
	static bool testBit(const unsigned char* bits, int code);

public:
	// ctor / dtor
	InputReader();
	~InputReader();

	// owns file descriptors, so no copies
	InputReader(const InputReader&) = delete;
	InputReader& operator=(const InputReader&) = delete;

	bool hasKeyboard() const;

	void poll();
	bool queryKey(int key) const;
};

}
