#pragma once

#include <InputReader.hpp>
#include <mutex>
#include <vector>
#include <deque>
#include <chrono>

namespace Input
{

// sleep between poll passes, a CPU governor only since movement is scaled by deltaTime
inline constexpr int pollIntervalMicroseconds = 1000;

// upper bound on deltaTime, so a stalled pass can't jump controls across their whole range
inline constexpr double maxDeltaTime = 0.1;

struct PrecisionMoverInput
{
	// settings
	int positiveKey;
	int negativeKey;

	double value = 0.0;

	// what RESET_MODIFIER_KEY + either direction key snaps this control back to
	double defaultValue = 0.0;

	// units of travel per second while held (not per poll pass)
	double speed = 0.1;

	double max = 1.0;
	double min = -1.0;

	void (*callback)(double);
};

struct MomentumMoverInput
{
	// settings
	int positiveKey;
	int negativeKey;

	double value = 0.0;

	// units of travel per second while held (not per poll pass)
	double speed = 0.1;

	// seconds for the value to fall halfway back to 0 once released
	double decayHalfLife = 0.05;

	double max = 1.0;
	double min = -1.0;

	void (*callback)(double);

	// internal
};

struct ButtonInput
{
	// settings
	int activationKey;

	void (*callback)();

	// internal
	bool pressed;
	bool lastPressed;
};

class InputInterpreter
{
private:
	InputReader* iReader = nullptr;

	// deque, not vector: std::mutex can't be moved, and deque never relocates elements on growth
	std::deque<std::mutex> precisionMoverLocks;
	std::deque<std::mutex> momentumMoverLocks;
	std::deque<std::mutex> buttonLocks;

	std::vector<PrecisionMoverInput> precisionMoverInputs;
	std::vector<MomentumMoverInput> momentumMoverInputs;
	std::vector<ButtonInput> buttonInputs;

	// movement logic, movers scale by deltaTime, buttons are edge triggered
	void processInput(PrecisionMoverInput* input);
	void processInput(MomentumMoverInput* input);
	void processInput(ButtonInput* input);

	// seconds the current pass covers, shared by every input; poll thread only, so no lock
	double deltaTime = 0.0;
	std::chrono::steady_clock::time_point previousCycle = std::chrono::steady_clock::now();

	// whether RESET_MODIFIER_KEY was down this pass
	bool resetModifierHeld = false;

	void advanceDeltaTime();
	void refreshKeyState();

	// for thread
	bool running = true;

	void updateAllMovers();
	static void moverUpdateLoop(InputInterpreter* self);

public:
	// ctor / dtor
	InputInterpreter();
	~InputInterpreter();

	void provideIReader(InputReader* iReader);
	void startRefreshThread();
	void stopRefreshThread();

	// input addition overloads
	void addInput(int positiveDirectionKey, int negativeDirectionKey,
		      double startVal, double speed,
		      double min, double max,
		      void (*callback)(double));

	void addInput(int positiveDirectionKey, int negativeDirectionKey,
		      double startVal, double speed, double decayHalfLife,
		      double min, double max,
		      void (*callback)(double));

	void addInput(int activationKey,
		      void (*callback)());
};

}
