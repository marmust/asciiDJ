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

inline int refreshRateMS = 1.0;

struct PrecisionMoverInput
{
	// settings
	char positiveKey;
	char negativeKey;

	double value = 0.0;
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
	double speed = 0.1;
	double decayRate = 0.1;

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
	InputReader* iReader;

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

	// movement logic
	void processInput(PrecisionMoverInput* input);
	void processInput(MomentumMoverInput* input);
	void processInput(ButtonInput* input);

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
		      double startVal, double speed, double decayRate,
		      double min, double max,
		      void (*callback)(double));

	void addInput(char activationKey,
		      void (*callback)());
};

}
