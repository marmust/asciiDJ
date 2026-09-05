#pragma once

#include <vector>
#include <Deck.hpp>
#include <DirReader.hpp>

namespace Files
{

class FileLoadManager
{
private:
	// non-owning, added in addDeck() call order (index 0 = the first deck added, etc)
	std::vector<Playback::Deck*> decks;

	std::vector<FileEntry> files;
	int selectedIdx = 0;

public:
	// ctor / dtor
	FileLoadManager() = default;
	~FileLoadManager() = default;

	void addDeck(Playback::Deck* deck);
	void reportFiles(std::vector<FileEntry> files);

	// loads files[fileIdx]'s fpath onto decks[deckIdx]; silently no-ops if either index is out
	// of range
	void loadToDeck(int deckIdx, int fileIdx);

	int getSelectedIdx() const;
	void setSelectedIdx(int idx);
};

}
