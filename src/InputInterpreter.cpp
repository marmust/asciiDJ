#include <InputInterpreter.hpp>
#include <InputSchema.hpp>

#include <algorithm>
#include <cmath>
#include <thread>

namespace Input
{

/// @brief ctor
InputInterpreter::InputInterpreter() {}

/// @brief dtor
InputInterpreter::~InputInterpreter()
{
	// stop the thread if someone forgot
	this->running = false;
}

/// @brief setter for an externally owned input reader instance
/// @param InputReader* iReader the iReader to read key presses from
void InputInterpreter::provideIReader(InputReader* iReader) 
{
        this->iReader = iReader;
}

/// @brief adds a new precision mover type input to be processed
/// @param int positiveDirection which keyboard key to be pressed to move the input in the positive direction
/// @param int negativeDirection which keyboard key to be pressed to move the input in the negative direction
/// @param double startVal initial value of the slider
/// @param double speed multiplier on the movement speed of the slider
/// @param double min the min allowed clamp value of the slider
/// @param double max the max allowed clamp value of the slider
/// @param void (*callback)(double) callback func ptr to receive the double slider value on move
void InputInterpreter::addInput(int positiveDirectionKey, int negativeDirectionKey,
	                        double startVal, double speed,
	                        double min, double max,
	                        void (*callback)(double))
{
	// startVal doubles as the default the reset modifier snaps back to
	PrecisionMoverInput newInput = { positiveDirectionKey, negativeDirectionKey,
					 startVal, startVal, speed, max, min, callback };

	// add it to the correct vec and assign it a mutex
	this->precisionMoverInputs.push_back(newInput);
	this->precisionMoverLocks.emplace_back();
}

/// @brief adds a new momentum mover type input to be processed
/// @param int positiveDirection which keyboard key to be pressed to move the input in the positive direction
/// @param int negativeDirection which keyboard key to be pressed to move the input in the negative direction
/// @param double startVal initial value of the slider
/// @param double speed multiplier on the movement speed of the slider
/// @param double min the min allowed clamp value of the slider
/// @param double max the max allowed clamp value of the slider
/// @param double decayHalfLife seconds for the value to fall halfway back to 0 while released
/// @param void (*callback)(double) callback func ptr to receive the double slider value on move
void InputInterpreter::addInput(int positiveDirectionKey, int negativeDirectionKey,
		                double startVal, double speed, double decayHalfLife,
		                double min, double max,
		                void (*callback)(double))
{
	// create the appropriate struct
	MomentumMoverInput newInput = { positiveDirectionKey, negativeDirectionKey,
					 startVal, speed, decayHalfLife,
					 max, min, callback };

	// add it to the correct vec and assign it a mutex
	this->momentumMoverInputs.push_back(newInput);
	this->momentumMoverLocks.emplace_back();
}

/// @brief adds a new momentum mover type input to be processed
/// @param int activationKey which keyboard key to be pressed to activate the button
/// @param void (*callback)() callback func ptr to be called on button activation
void InputInterpreter::addInput(int activationKey, void (*callback)())
{
	// create the appropriate struct
	ButtonInput newInput = { activationKey, callback };

	// add it to the correct vec and assign it a mutex
	this->buttonInputs.push_back(newInput);
	this->buttonLocks.emplace_back();
}

/// @brief updates a precision mover input from the iReader and reports to its callback
/// @param PrecisionMoverInput* input input pointer to which input to update / report
void InputInterpreter::processInput(PrecisionMoverInput* input)
{
	// get the current movement direction by polling the iReader
	double direction = 0.0;

	bool positivePressed = this->iReader->queryKey(input->positiveKey);
	bool negativePressed = this->iReader->queryKey(input->negativeKey);

	// both pressed = stays at 0
	direction += positivePressed ? 1.0 : 0;
	direction += negativePressed ? -1.0 : 0;

	// speed is units per second, so scale by the time this pass covers
	direction *= input->speed * this->deltaTime;

	// reset modifier held: snap to the default instead of nudging
	if (this->resetModifierHeld && (positivePressed || negativePressed))
		input->value = input->defaultValue;
	else
		input->value += direction;

	input->value = std::clamp(input->value, input->min, input->max);

	// if the input is held down report to callback
	if (positivePressed || negativePressed)
		input->callback(input->value);
}

/// @brief updates a momentum mover input from the iReader and reports to its callback
/// @param MomentumMoverInput* input input pointer to which input to update / report
void InputInterpreter::processInput(MomentumMoverInput* input)
{
	// get the current movement direction by polling the iReader
	double direction = 0.0;

	bool positivePressed = this->iReader->queryKey(input->positiveKey);
	bool negativePressed = this->iReader->queryKey(input->negativeKey);

	// both pressed = stays at 0
	direction += positivePressed ? 1.0 : 0;
	direction += negativePressed ? -1.0 : 0;

	// speed is units per second, same as the precision mover
	direction *= input->speed * this->deltaTime;

	// apply the direction over the input
	if (this->resetModifierHeld && (positivePressed || negativePressed))
	{
		// a momentum control rests at 0
		input->value = 0.0;
	}
	else if (positivePressed || negativePressed)
	{
		input->value += direction;
	}
	else if (input->decayHalfLife > 0.0)
	{
		// exponential decay by half-life, independent of the loop rate
		input->value *= std::pow(0.5, this->deltaTime / input->decayHalfLife);
	}
	else
	{
		// a non-positive half-life means "snap straight back"
		input->value = 0.0;
	}

	// apply clamp
	input->value = std::clamp(input->value, input->min, input->max);

	// always report to callback, the value decays while released
	input->callback(input->value);
}

/// @brief updates a button input from the iReader and fires its callback on press
/// @param ButtonInput* input input pointer to which input to update / report
void InputInterpreter::processInput(ButtonInput* input)
{
	input->pressed = this->iReader->queryKey(input->activationKey);

	// fire on the press edge only
	if (input->pressed && input->pressed != input->lastPressed)
		input->callback();

	// progress the trailing param
	input->lastPressed = input->pressed;
}

/// @brief runs one polling pass over every registered input, call advanceDeltaTime() first
void InputInterpreter::updateAllMovers()
{
	for (int currentInputIdx = 0; currentInputIdx < this->precisionMoverInputs.size(); currentInputIdx++)
	{
		std::lock_guard<std::mutex> lock(this->precisionMoverLocks[currentInputIdx]);
		this->processInput(&this->precisionMoverInputs[currentInputIdx]);
	}

	for (int currentInputIdx = 0; currentInputIdx < this->momentumMoverInputs.size(); currentInputIdx++)
	{
		std::lock_guard<std::mutex> lock(this->momentumMoverLocks[currentInputIdx]);
		this->processInput(&this->momentumMoverInputs[currentInputIdx]);
	}

	for (int currentInputIdx = 0; currentInputIdx < this->buttonInputs.size(); currentInputIdx++)
	{
		std::lock_guard<std::mutex> lock(this->buttonLocks[currentInputIdx]);
		this->processInput(&this->buttonInputs[currentInputIdx]);
	}
}

/// @brief rolls deltaTime forward to cover the pass about to be processed, call once per pass
void InputInterpreter::advanceDeltaTime()
{
	auto now = std::chrono::steady_clock::now();

	// clamped to maxDeltaTime so a stalled pass can't jump controls
	this->deltaTime = std::min(std::chrono::duration<double>(now - this->previousCycle).count(),
				    Input::maxDeltaTime);

	this->previousCycle = now;
}

/// @brief snapshots the keyboard once for the pass about to be processed
void InputInterpreter::refreshKeyState()
{
	this->iReader->poll();
	this->resetModifierHeld = this->iReader->queryKey(RESET_MODIFIER_KEY);
}

/// @brief poll thread loop, one pass per pollIntervalMicroseconds
/// @param InputInterpreter* self this pointer to accomodate threads being static funcs only
void InputInterpreter::moverUpdateLoop(InputInterpreter* self)
{
	while (self->running)
	{
		self->advanceDeltaTime();
		self->refreshKeyState();
		self->updateAllMovers();

		// CPU governor only, deltaTime keeps feel independent of the loop rate
		std::this_thread::sleep_for(std::chrono::microseconds(Input::pollIntervalMicroseconds));
	}
}

/// @brief func to launch the refresh thread and detach
void InputInterpreter::startRefreshThread()
{
	// rebase the clock so the first pass doesn't get a huge deltaTime
	this->previousCycle = std::chrono::steady_clock::now();
	this->deltaTime = 0.0;

	this->running = true;
	std::thread refreshThread(this->moverUpdateLoop, this);
	refreshThread.detach();
}

/// @brief stops refresh thread
void InputInterpreter::stopRefreshThread()
{
	this->running = false;
}

}
