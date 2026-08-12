#pragma once

#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace Input
{

class InputReader
{
private:
	Display* dpy;

public:
	// ctor / dtor
	InputReader();
	~InputReader();

	bool queryKey(char key);
};

}
