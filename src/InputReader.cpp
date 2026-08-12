#include <InputReader.hpp>

namespace Input
{

/// @brief ctor
InputReader::InputReader()
{
	this->dpy = XOpenDisplay(nullptr);
}

/// @brief dtor
InputReader::~InputReader()
{
	if (this->dpy)
		XCloseDisplay(this->dpy);
}

/// @brief function to query whether a key is down right now or not
/// @param char key which keyboard key to query
/// @returns bool true->pressed false->released
bool InputReader::queryKey(char key)
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

}
