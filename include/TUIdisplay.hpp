#pragma once

#include <GraphicsComposer.hpp>
#include <Deck.hpp>
#include <Mixer.hpp>
#include <Equalizer.hpp>

namespace Graphics
{

struct CursorPos
{
	int x = 0;
	int y = 0;
};

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

	CursorPos startPos;
	GraphicsComposer* composer = nullptr;

	CursorPos queryCursorPos();

	// to / from composer
	TUItelemetry gatherTUItelemetry();
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

	void startUpdateLoop();
	void stopUpdateLoop();

	void moveCursorToPos(CursorPos pos);
};

}
