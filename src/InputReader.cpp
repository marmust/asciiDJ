#include <InputReader.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <iostream>

namespace Input
{

/// @brief tests one key code's bit in an evdev key bitmap
/// @param const unsigned char* bits the bitmap to test (one bit per evdev key code)
/// @param int code which key code's bit to test
/// @returns bool true if the bit is set
bool InputReader::testBit(const unsigned char* bits, int code)
{
	return (bits[code / 8] & (1 << (code % 8))) != 0;
}

/// @brief ctor
InputReader::InputReader()
{
	this->openKeyboards();
}

/// @brief dtor
InputReader::~InputReader()
{
	for (int fd : this->keyboardFds)
		close(fd);
}

/// @brief opens every /dev/input/event* device that reports EV_KEY with the letter/space keys
void InputReader::openKeyboards()
{
	bool permissionDenied = false;

	for (int i = 0; i < 32; i++)
	{
		std::string path = "/dev/input/event" + std::to_string(i);
		int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);

		if (fd < 0)
		{
			permissionDenied |= (errno == EACCES);
			continue;
		}

		unsigned long evBits = 0;
		ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), &evBits);

		unsigned char keyBits[(KEY_MAX + 7) / 8] = {0};
		ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits);

		bool isKeyboard = (evBits & (1UL << EV_KEY)) &&
				  testBit(keyBits, KEY_A) && testBit(keyBits, KEY_Z) && testBit(keyBits, KEY_SPACE);

		if (isKeyboard)
			this->keyboardFds.push_back(fd);
		else
			close(fd);
	}

	if (!this->keyboardFds.empty())
		return;

	if (permissionDenied)
		std::cerr << "inputreader: no permission to read /dev/input/event*. run:\n"
			  << "    sudo usermod -aG input $USER\n"
			  << "then log out and back in" << std::endl;
	else
		std::cerr << "inputreader: found no keyboard under /dev/input" << std::endl;
}

/// @brief whether at least one keyboard was opened; if not, every key reads as released
/// @returns bool true if input is available
bool InputReader::hasKeyboard() const
{
	return !this->keyboardFds.empty();
}

/// @brief refreshes the key state snapshot, call once per poll pass before queryKey()
void InputReader::poll()
{
	std::memset(this->keyState, 0, sizeof(this->keyState));

	for (int fd : this->keyboardFds)
	{
		unsigned char deviceState[sizeof(this->keyState)] = {0};

		if (ioctl(fd, EVIOCGKEY(sizeof(deviceState)), deviceState) < 0)
			continue;

		for (size_t i = 0; i < sizeof(this->keyState); i++)
			this->keyState[i] |= deviceState[i];
	}
}

/// @brief whether a key was down as of the last poll()
/// @param int key evdev key code (KEY_* from linux/input-event-codes.h)
/// @returns bool true->pressed false->released
bool InputReader::queryKey(int key) const
{
	if (key < 0 || key > KEY_MAX)
		return false;

	return testBit(this->keyState, key);
}

}
