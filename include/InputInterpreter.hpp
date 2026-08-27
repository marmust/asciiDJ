#pragma once

#include <InputReader.hpp>
#include <mutex>
#include <vector>
#include <deque>
#include <algorithm>
#include <chrono>
#include <thread>

namespace Input
{

// how long the poll loop sleeps between passes. this is ONLY a CPU governor - movement is
// scaled by deltaTime, so feel no longer depends on how fast this loop runs. left in because
// a free running poll loop hits ~130kHz here, ie millions of queryKey() syscalls a second
// pegging a core for nothing
inline constexpr int pollIntervalMicroseconds = 1000;

// deltaTime is clamped to this, so a scheduling hiccup (or a suspended process) can't teleport
// every control across its whole range on the pass that follows - same idea as Unity's
// Time.maximumDeltaTime
inline constexpr double maxDeltaTime = 0.1;

struct PrecisionMoverInput
{
	// settings
	char positiveKey;
	char negativeKey;

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
	char positiveKey;
	char negativeKey;

	double value = 0.0;

	// units of travel per second while held (not per poll pass)
	double speed = 0.1;

	// seconds for the value to fall halfway back to 0 once released; a rate per pass would
	// mean nothing now that passes aren't a fixed length
	double decayHalfLife = 0.05;

	double max = 1.0;
	double min = -1.0;

	void (*callback)(double);

	// internal
};

struct ButtonInput
{
	// settings
	char activationKey;

	void (*callback)();

	// internal
	bool pressed;
	bool lastPressed;
};

class InputInterpreter
{
private:
	InputReader* iReader = nullptr;

	// "well known pattern" my ass this is an abomination
	// deque, not vector: std::mutex is neither movable nor copyable, so growing a
	// vector<mutex> past capacity fails to compile on reallocation; deque never
	// relocates existing elements when it grows, so it works here
	std::deque<std::mutex> precisionMoverLocks;
	std::deque<std::mutex> momentumMoverLocks;
	std::deque<std::mutex> buttonLocks;

	std::vector<PrecisionMoverInput> precisionMoverInputs;
	std::vector<MomentumMoverInput> momentumMoverInputs;
	std::vector<ButtonInput> buttonInputs;

	// movement logic; every mover scales its travel by this->deltaTime automatically, so
	// nothing here has to be handed a time step. buttons are edge triggered and ignore it
	void processInput(PrecisionMoverInput* input);
	void processInput(MomentumMoverInput* input);
	void processInput(ButtonInput* input);

	// timing state, touched only by the poll thread (advanceDeltaTime writes it at the top of
	// each pass, the processInput overloads read it further down that same pass), so it needs
	// no lock of its own unlike the per input state above
	//
	// seconds of wall clock the pass currently being processed covers. one value for the whole
	// pass: every mover is updated microseconds after every other one on that one thread, so
	// they all cover the identical window and a per input clock would just duplicate this
	double deltaTime = 0.0;
	std::chrono::steady_clock::time_point previousCycle = std::chrono::steady_clock::now();

	// whether RESET_MODIFIER_KEY was down when this pass started. polled once per pass for the
	// same reason deltaTime is: it's one key, and querying it per input would fire the same
	// syscall once for every registered control instead of once for all of them
	bool resetModifierHeld = false;

	void advanceDeltaTime();
	void refreshResetModifier();

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
	void addInput(char positiveDirectionKey, char negativeDirectionKey,
		      double startVal, double speed,
		      double min, double max,
		      void (*callback)(double));

	void addInput(char positiveDirectionKey, char negativeDirectionKey,
		      double startVal, double speed, double decayHalfLife,
		      double min, double max,
		      void (*callback)(double));

	void addInput(char activationKey,
		      void (*callback)());
};

}
