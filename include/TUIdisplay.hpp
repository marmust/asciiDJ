#pragma once

#include <GraphicsComposer.hpp>
#include <Deck.hpp>
#include <Mixer.hpp>
#include <Equalizer.hpp>
#include <DirReader.hpp>
#include <FileLoadManager.hpp>
#include <string>
#include <vector>
#include <memory>

// forward declared, termios.h #defines macros (eg SPEED_MAX) that collide with Input constants
struct termios;

namespace Graphics
{

struct CursorPos
{
	int x = 0;
	int y = 0;
};

// redraw interval of TUIupdateLoop (~60fps)
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

	// terminal settings from before disableTerminalEcho(), restored by the dtor
	std::unique_ptr<termios> originalTermios;
	void disableTerminalEcho();

	// file-select mode flag, picks which compose function composeCurrentFrame() calls
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
