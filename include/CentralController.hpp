#pragma once

#include <Deck.hpp>
#include <Mixer.hpp>
#include <Equalizer.hpp>
#include <InputReader.hpp>
#include <InputInterpreter.hpp>
#include <FileLoadManager.hpp>
#include <DirReader.hpp>
#include <TUIdisplay.hpp>

// that name is sooo fucking tuff
namespace CommandAndControl
{

class CentralController
{
private:
	// audio, hardwired to two decks (L/R) although the mixer supports n
	Playback::Deck* deck1 = nullptr;
	Playback::Deck* deck2 = nullptr;

	Playback::Equalizer* eq1 = nullptr;
	Playback::Equalizer* eq2 = nullptr;

	Playback::Mixer* mixer = nullptr;

	// control
	Input::InputReader* iReader = nullptr;
	Input::InputInterpreter* interpreter = nullptr;

	// file browser
	Files::FileLoadManager* fileManager = nullptr;
	Graphics::TUIdisplay* display = nullptr;
	Files::DirReader* dirReader = nullptr;

	// file-select mode flag, mirrored into display's toggleFileDisplay on every toggle
	bool fileSelectionMode = false;

	// callbacks are plain function pointers, so they reach the instance through this
	static inline CentralController* self = nullptr;

	// InputInterpreter callbacks: decks
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

	// file browser
	static void fileSelectToggleCallback();

	static void arrowUpCallback();
	static void arrowDownCallback();
	static void arrowLeftCallback();
	static void arrowRightCallback();

	// setup
	void configureInputs();
	void addEQbands();
	void resetEQs();

public:
	// ctor / dtor
	CentralController(Playback::Deck* deck1, Playback::Deck* deck2,
			   Playback::Equalizer* eq1, Playback::Equalizer* eq2,
			   Playback::Mixer* mixer,
			   Input::InputReader* iReader, Input::InputInterpreter* interpreter,
			   Files::FileLoadManager* fileManager, Graphics::TUIdisplay* display,
			   Files::DirReader* dirReader);

	~CentralController() = default;

	void startRun();
	void stopRun();
};

}
