#pragma once

#include <Deck.hpp>
#include <Mixer.hpp>
#include <Equalizer.hpp>
#include <InputReader.hpp>
#include <InputInterpreter.hpp>

// that name is sooo fucking tuff
namespace CommandAndControl
{

class CentralController
{
private:
	// audio
	// even though mixer supports n-deck hardwire L/R
	Playback::Deck* deck1 = nullptr;
	Playback::Deck* deck2 = nullptr;

	Playback::Equalizer* eq1 = nullptr;
	Playback::Equalizer* eq2 = nullptr;

	Playback::Mixer* mixer = nullptr;

	// control
	Input::InputReader* iReader = nullptr;
	Input::InputInterpreter* interpreter = nullptr;

	// InputInterpreter callbacks are plain function pointers (no way to carry captured
	// context), so these have to be static; self points back at the instance they act on
	static CentralController* self;

	// InputInterpreter callbacks
	// decks
	static void deck1SpeedCallback(double speed);
	static void deck2SpeedCallback(double speed);

	static void deck1ShiftCallback(double shift);
	static void deck2ShiftCallback(double shift);

	static void deck1pauseCallback();
	static void deck2pauseCallback();

	// mixer
	static void deck1VolumeCallback(double volume);
	static void deck2VolumeCallback(double volume);
	static void xfaderCallback(double xfaderPos);

	// eqs
	static void deck1EQbassCallback(double db);
	static void deck2EQbassCallback(double db);

	static void deck1EQmidsCallback(double db);
	static void deck2EQmidsCallback(double db);

	static void deck1EQhighCallback(double db);
	static void deck2EQhighCallback(double db);

	// setup
	void configureInputs();
	void addEQbands();
	void resetEQs();

public:
	// ctor / dtor
	CentralController(Playback::Deck* deck1, Playback::Deck* deck2,
			   Playback::Equalizer* eq1, Playback::Equalizer* eq2,
			   Playback::Mixer* mixer,
			   Input::InputReader* iReader, Input::InputInterpreter* interpreter);

	~CentralController() = default;

	void startRun();
	void stopRun();
};

}
