#pragma once

#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace Input
{

class InputReader
{
private:
	// X11 backend - only used when not running as root; under Wayland this backend can't
	// see real key state unless it belongs to a focused X11/XWayland window (which this
	// reader never creates), so it's best-effort
	Display* dpy = nullptr;

	// evdev backend - used when running as root, since reading /dev/input/event* needs
	// that; reads live key state straight from the kernel, unaffected by X11 vs Wayland or
	// window focus entirely
	int evdevFd = -1;

	bool isRoot();
	void openEvdevKeyboard();

	bool queryKeyX11(int key);
	bool queryKeyEvdev(int key);

public:
	// ctor / dtor
	InputReader();
	~InputReader();

	bool queryKey(int key);
};

}
