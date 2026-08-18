#include <InputReader.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <string>
#include <iostream>

namespace Input
{

namespace
{
	// evdev key codes aren't ASCII, so every char InputSchema.hpp actually binds to a
	// control needs an explicit mapping here; anything unmapped returns -1
	int charToEvdevKeyCode(char c)
	{
		switch (c)
		{
			case 'a': return KEY_A;
			case 'b': return KEY_B;
			case 'c': return KEY_C;
			case 'd': return KEY_D;
			case 'e': return KEY_E;
			case 'f': return KEY_F;
			case 'g': return KEY_G;
			case 'h': return KEY_H;
			case 'i': return KEY_I;
			case 'j': return KEY_J;
			case 'k': return KEY_K;
			case 'l': return KEY_L;
			case 'm': return KEY_M;
			case 'n': return KEY_N;
			case 'o': return KEY_O;
			case 'p': return KEY_P;
			case 'q': return KEY_Q;
			case 'r': return KEY_R;
			case 's': return KEY_S;
			case 't': return KEY_T;
			case 'u': return KEY_U;
			case 'v': return KEY_V;
			case 'w': return KEY_W;
			case 'x': return KEY_X;
			case 'y': return KEY_Y;
			case 'z': return KEY_Z;
			case ';': return KEY_SEMICOLON;
			case ',': return KEY_COMMA;
			case '.': return KEY_DOT;
			default: return -1;
		}
	}
}

/// @brief ctor
InputReader::InputReader()
{
	if (this->isRoot())
		this->openEvdevKeyboard();
	else
		this->dpy = XOpenDisplay(nullptr);
}

/// @brief dtor
InputReader::~InputReader()
{
	if (this->dpy)
		XCloseDisplay(this->dpy);

	if (this->evdevFd >= 0)
		close(this->evdevFd);
}

/// @brief quick check for whether this process is running as root - only root can read
/// /dev/input/event* for the evdev backend
/// @returns bool true if running as root
bool InputReader::isRoot()
{
	return geteuid() == 0;
}

/// @brief scans /dev/input/event* for the first device that looks like a real keyboard
/// (reports EV_KEY and supports the letter/space keys, unlike e.g. a mouse or a power
/// button that also happens to expose EV_KEY for one or two codes) and keeps it open
void InputReader::openEvdevKeyboard()
{
	for (int i = 0; i < 32; i++)
	{
		std::string path = "/dev/input/event" + std::to_string(i);
		int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);

		if (fd < 0)
			continue;

		unsigned long evBits = 0;
		ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), &evBits);

		if (!(evBits & (1UL << EV_KEY)))
		{
			close(fd);
			continue;
		}

		unsigned char keyBits[(KEY_MAX + 7) / 8] = {0};
		ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits);

		auto hasKey = [&](int code) { return (keyBits[code / 8] & (1 << (code % 8))) != 0; };

		if (hasKey(KEY_A) && hasKey(KEY_Z) && hasKey(KEY_SPACE))
		{
			this->evdevFd = fd;
			return;
		}

		close(fd);
	}

	std::cout << "inputreader: running as root but found no keyboard-capable /dev/input device" << std::endl;
}

/// @brief function to query whether a key is down right now or not
/// @param char key which keyboard key to query
/// @returns bool true->pressed false->released
bool InputReader::queryKey(char key)
{
	if (this->evdevFd >= 0)
		return this->queryKeyEvdev(key);

	return this->queryKeyX11(key);
}

/// @brief X11 backend for queryKey() - best-effort: XQueryKeymap only reflects real key
/// state when this process's X connection belongs to a focused X11/XWayland window, which
/// this reader never creates, so under Wayland this generally reports nothing pressed
bool InputReader::queryKeyX11(char key)
{
	if (!this->dpy)
		return false;

	KeyCode kc = XKeysymToKeycode(dpy, (KeySym)key);

	if (kc == 0)
		return false;

	char keys[32];
	XQueryKeymap(dpy, keys);

	return (keys[kc / 8] & (1 << (kc % 8))) != 0;
}

/// @brief evdev backend for queryKey() - reads live key state directly from the kernel via
/// EVIOCGKEY, independent of X11/Wayland and window focus entirely
bool InputReader::queryKeyEvdev(char key)
{
	if (this->evdevFd < 0)
		return false;

	int code = charToEvdevKeyCode(key);

	if (code < 0)
		return false;

	unsigned char keyState[(KEY_MAX + 7) / 8] = {0};

	if (ioctl(this->evdevFd, EVIOCGKEY(sizeof(keyState)), keyState) < 0)
		return false;

	return (keyState[code / 8] & (1 << (code % 8))) != 0;
}

}
