#include <InputInterpreter.hpp>

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
/// @param char positiveDirection which keyboard key to be pressed to move the input in the positive direction
/// @param char negativeDirection which keyboard key to be pressed to move the input in the negative direction
/// @param double startVal initial value of the slider
/// @param double speed multiplier on the movement speed of the slider
/// @param double min the min allowed clamp value of the slider
/// @param double max the max allowed clamp value of the slider
/// @param void (*callback)(double) callback func ptr to receive the double slider value on move
void InputInterpreter::addInput(char positiveDirectionKey, char negativeDirectionKey,
	                        double startVal, double speed,
	                        double min, double max,
	                        void (*callback)(double))
{
	// create the appropriate struct
	PrecisionMoverInput newInput = { positiveDirectionKey, negativeDirectionKey,
					 startVal, speed, max, min, callback };

	// add it to the correct vec and assign it a mutex
	this->precisionMoverInputs.push_back(newInput);
	this->precisionMoverLocks.emplace_back();
}

/// @brief adds a new momentum mover type input to be processed
/// @param char positiveDirection which keyboard key to be pressed to move the input in the positive direction
/// @param char negativeDirection which keyboard key to be pressed to move the input in the negative direction
/// @param double startVal initial value of the slider
/// @param double speed multiplier on the movement speed of the slider
/// @param double min the min allowed clamp value of the slider
/// @param double max the max allowed clamp value of the slider
/// @param double decayRate multiplier applied to the value each tick while released (pulls it back toward 0)
/// @param void (*callback)(double) callback func ptr to receive the double slider value on move
void InputInterpreter::addInput(char positiveDirectionKey, char negativeDirectionKey,
		                double startVal, double speed, double decayRate,
		                double min, double max,
		                void (*callback)(double))
{
	// create the appropriate struct
	MomentumMoverInput newInput = { positiveDirectionKey, negativeDirectionKey,
					 startVal, speed, decayRate,
					 max, min, callback };

	// add it to the correct vec and assign it a mutex
	this->momentumMoverInputs.push_back(newInput);
	this->momentumMoverLocks.emplace_back();
}

/// @brief adds a new momentum mover type input to be processed
/// @param char activationKey which keyboard key to be pressed to activate the button
/// @param void (*callback)() callback func ptr to be called on button activation
void InputInterpreter::addInput(char activationKey, void (*callback)())
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
        void processInput(PrecisionMoverInput* input);

	// both pressed = stays at 0
	direction += positivePressed ? 1.0 : 0;
	direction += negativePressed ? -1.0 : 0;

	// apply speed
	direction *= input->speed;

	// apply the direction over the input and clamp to range
	input->value += direction;
	input->value = std::clamp(input->value, input->min, input->max);

	// if the input is held down report to callback
	if (positivePressed || negativePressed)
		input->callback(input->value);
}

/// @brief function to  update a given precision mover input based on the report of the iReader and report to input's callback
/// @param PrecisionMoverInput* input input pointer to which input to update / report
void InputInterpreter::processInput(MomentumMoverInput* input)
{
	// get the current movement direction by polling the iReader
	double direction = 0.0;

	bool positivePressed = this->iReader->queryKey(input->positiveKey);
	bool negativePressed = this->iReader->queryKey(input->negativeKey);

	// both pressed = stays at 0
	direction += positivePressed ? 1.0 : 0;
	direction += negativePressed ? -1.0 : 0;

	// apply speed and momentum
	direction *= input->speed;

	// apply the direction over the input
	if (positivePressed || negativePressed)
		input->value += direction;
	else
		input->value *= input->decayRate;

	// apply clamp
	input->value = std::clamp(input->value, input->min, input->max);

	// for now always report to callback
	input->callback(input->value);
}

/// @brief runs one polling pass over every registered input, updating state and firing callbacks as needed
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
}

/// @brief thread func that runs at a fixed loop speed and updates the inputs
/// @param InputInterpreter* self this pointer to accomodate threads being static funcs only
void InputInterpreter::moverUpdateLoop(InputInterpreter* self)
{
	while (self->running)
	{
		auto cycleStart = std::chrono::steady_clock::now();

		// update all inputs, then sleep off however much of the refresh period is left over
		// so the updates stay evenly spaced regardless of how long this cycle took
		self->updateAllMovers();

		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - cycleStart);
		auto sleepFor = std::chrono::milliseconds(Input::refreshRateMS) - elapsed;

		if (sleepFor > std::chrono::milliseconds::zero())
			std::this_thread::sleep_for(sleepFor);
	}
}

/// @brief func to launch the refresh thread and detach
void InputInterpreter::startRefreshThread()
{
	this->running = true;
	std::thread refreshThread(this->moverUpdateLoop, this);
	refreshThread.detach();
}

/// @brief stops refresh thread
void InputInterpreter::stopRefreshThread()
{
	this->running = false;
}

/// @brief manual single-shot poll, see header for usage
void InputInterpreter::update()
{
	this->updateAllMovers();
}

}
