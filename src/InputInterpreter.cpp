#include <InputInterpreter.hpp>
#include <InputTuning.hpp>

#include <cmath>

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
	// create the appropriate struct
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

/// @brief function to  update a given precision mover input based on the report of the iReader and report to input's callback
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

	// speed is units per second, so scale by however long this pass actually covered - the
	// same key held for the same real time travels the same distance at any loop rate
	direction *= input->speed * this->deltaTime;

	// touching the control with the reset modifier down hard sets it to its default instead
	// of nudging it, for as long as both stay held
	if (this->resetModifierHeld && (positivePressed || negativePressed))
		input->value = input->defaultValue;
	else
		input->value += direction;

	input->value = std::clamp(input->value, input->min, input->max);

	// if the input is held down report to callback
	if (positivePressed || negativePressed)
		input->callback(input->value);
}

/// @brief function to update a given momentum mover input based on the report of the iReader and report to input's callback
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
		// a momentum control's rest position is dead centre, so reset means straight to 0.
		// ahead of the decay arm below, so it parks there instead of sagging off it
		input->value = 0.0;
	}
	else if (positivePressed || negativePressed)
	{
		input->value += direction;
	}
	else if (input->decayHalfLife > 0.0)
	{
		// exponential decay expressed as a half-life, so the value released from any
		// position takes the same real time to fall halfway back to 0 no matter how many
		// passes the loop fits into that time
		input->value *= std::pow(0.5, this->deltaTime / input->decayHalfLife);
	}
	else
	{
		// a non-positive half-life means "snap straight back"
		input->value = 0.0;
	}

	// apply clamp
	input->value = std::clamp(input->value, input->min, input->max);

	// for now always report to callback
	input->callback(input->value);
}

/// @brief function to update a given button input based on the report of the iReader and report to input's callback
/// @param ButtonInput* input input pointer to which input to update / report
void InputInterpreter::processInput(ButtonInput* input)
{
	input->pressed = this->iReader->queryKey(input->activationKey);

	// callback only if the button is PRESSED and not RELEASED and there was a difference between the current and trailing status
	if (input->pressed && input->pressed != input->lastPressed)
		input->callback();

	// progress the trailing param
	input->lastPressed = input->pressed;
}

/// @brief runs one polling pass over every registered input, updating state and firing callbacks
/// as needed; each mover picks up this->deltaTime on its own, so call advanceDeltaTime() first
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

/// @brief rolls this->deltaTime forward to cover the pass about to be processed, to be called
/// once at the top of every pass; every mover in that pass then scales its travel by it
void InputInterpreter::advanceDeltaTime()
{
	auto now = std::chrono::steady_clock::now();

	// clamped so a scheduling hiccup (or a suspended process) can't teleport every control
	// across its whole range on the pass that follows
	this->deltaTime = std::min(std::chrono::duration<double>(now - this->previousCycle).count(),
				    Input::maxDeltaTime);

	this->previousCycle = now;
}

/// @brief samples the reset modifier once for the pass about to be processed, so every mover in
/// it reads one key state instead of each re-querying the same key
void InputInterpreter::refreshResetModifier()
{
	this->resetModifierHeld = this->iReader != nullptr && this->iReader->queryKey(RESET_MODIFIER_KEY);
}

/// @brief thread func that polls the inputs as fast as the governor allows; deltaTime is rolled
/// forward once per pass so travel is measured in real seconds rather than in poll passes
/// @param InputInterpreter* self this pointer to accomodate threads being static funcs only
void InputInterpreter::moverUpdateLoop(InputInterpreter* self)
{
	while (self->running)
	{
		self->advanceDeltaTime();
		self->refreshResetModifier();
		self->updateAllMovers();

		// governor only: deltaTime already decouples feel from the loop rate, this just keeps
		// the poll loop from pegging a core on queryKey() syscalls
		std::this_thread::sleep_for(std::chrono::microseconds(Input::pollIntervalMicroseconds));
	}
}

/// @brief func to launch the refresh thread and detach
void InputInterpreter::startRefreshThread()
{
	// rebase the clock, so however long sat between construction and this call isn't charged
	// to the first pass as one big deltaTime
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
