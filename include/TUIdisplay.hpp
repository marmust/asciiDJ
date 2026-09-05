#pragma once

#include <GraphicsComposer.hpp>
#include <Deck.hpp>
#include <Mixer.hpp>
#include <Equalizer.hpp>
#include <DirReader.hpp>
#include <FileLoadManager.hpp>
#include <chrono>
#include <memory>

// forward declared rather than #include <termios.h> here - that header #defines macros (eg
// SPEED_MAX, a baud rate constant) that collide with unrelated same-named constants (eg
// Input::SPEED_MAX) in any file that includes this header transitively
struct termios;

namespace Graphics
{

struct CursorPos
{
	int x = 0;
	int y = 0;
};

// governs TUIupdateLoop's redraw rate - purely a CPU governor (60fps is already far smoother
// than a terminal needs), not tied to any control feel like InputInterpreter's poll interval is
inline constexpr int tuiFrameIntervalMicroseconds = 16666;

class TUIdisplay
{
private:
	// pointers to all telemetry gathering points

	// 2 decks, mixer, eqs, to fill out telstruct
	Playback::Deck* deck1 = nullptr;
	Playback::Deck* deck2 = nullptr;
	Playback::Mixer* mixer = nullptr;
	Playback::Equalizer* eq1 = nullptr;
	Playback::Equalizer* eq2 = nullptr;

	Files::DirReader* dirReader = nullptr;
	Files::FileLoadManager* fileManager = nullptr;

	CursorPos startPos;
	GraphicsComposer* composer = nullptr;

	// the terminal's settings from before disableTerminalEcho() touched them, so the dtor can
	// put the user's shell back to normal (echoing, canonical) on exit. unique_ptr (rather than
	// a plain member) since termios is only forward declared here
	std::unique_ptr<termios> originalTermios;
	void disableTerminalEcho();

	// which compose function composeCurrentFrame() calls - own copy of the mode flag (mirrored
	// from CentralController's fileSelectionMode via setToggleFileDisplay(), since TUIdisplay
	// and CentralController are independent peers with no pointer to each other otherwise)
	bool toggleFileDisplay = false;

	CursorPos queryCursorPos();

	// to / from composer
	TUItelemetry gatherTUItelemetry();
	void composeCurrentFrame();
	void flushBufferToConsole(const std::vector<std::string>* buffer);
	int calcFrameHeight(const std::vector<std::string>* buffer);
	void reserveRenderingSpace(int height, bool returnToTop = true);

	bool running = true;
	static void TUIupdateLoop(TUIdisplay* self);

public:
	// ctor / dtor
	TUIdisplay();
	~TUIdisplay();

	void setGraphicsComposer(GraphicsComposer* composer);

	void setDeck1(Playback::Deck* deck1);
	void setDeck2(Playback::Deck* deck2);
	void setMixer(Playback::Mixer* mixer);
	void setEq1(Playback::Equalizer* eq1);
	void setEq2(Playback::Equalizer* eq2);

	void setDirReader(Files::DirReader* dirReader);
	void setFileManager(Files::FileLoadManager* fileManager);

	void setToggleFileDisplay(bool toggleFileDisplay);

	void startUpdateLoop();
	void stopUpdateLoop();

	void moveCursorToPos(CursorPos pos);
};

}
