#pragma once

#include <Deck.hpp>
#include <Equalizer.hpp>
#include <vector>
#include <utility>
#include <algorithm>

namespace Playback
{

struct DeckRidealong
{
	// deck and eq instances
	Deck* deck;
	Equalizer* eq;

	// everything the mixer needs to control
	std::atomic<ma_double> volume = 1.0;

	DeckRidealong(Deck* deck, Equalizer* eq) : deck(deck), eq(eq) {}
};

class Mixer
{
private:
	// held in a vector, limited to 2 currently
	std::vector<std::unique_ptr<DeckRidealong>> decks;

	// works on even / odd decks, -1 = 0th (usually left) deck plays
	double crossfader = 0.0;

	std::pair<std::vector<std::unique_ptr<DeckRidealong>>*, double*> callbackData;

        // miniaudio backend
        ma_device device;
	bool isPlaying = false;

public:
	// ctor / dtor
	Mixer();
	~Mixer() = default;

	void addDeck(Deck* newDeck, Equalizer* eq);

	// miniaudio backend
	static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

	// state controls
	void play();
	void stop();

	// external controls
	void setVolume(double volume, int deckIdx);
	void setXfader(double xfaderPos);

	// state accessors
	double getVolume(int deckIdx) const;
	double getXfader() const;
};

}
