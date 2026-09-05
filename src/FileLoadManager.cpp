#include <FileLoadManager.hpp>

#include <algorithm>
#include <thread>

namespace Files
{

/// @brief registers a deck this manager can load files onto
/// @param Playback::Deck* deck non-owning deck ptr, indexed by call order (first added = 0, etc)
void FileLoadManager::addDeck(Playback::Deck* deck)
{
	this->decks.push_back(deck);
}

/// @brief replaces the file listing this manager picks from (eg from DirReader::listLocalFiles())
/// @param std::vector<FileEntry> files the new listing
void FileLoadManager::reportFiles(std::vector<FileEntry> files)
{
	this->files = std::move(files);
}

/// @brief loads files[fileIdx]'s fpath onto decks[deckIdx]; silently no-ops if either index is
/// out of range (no track selected / no such deck registered). Deck::loadTrack() is a full
/// synchronous decode, and this is called straight off the InputInterpreter poll thread (the one
/// thread driving every other control too) - fire it off on its own detached thread instead of
/// blocking that thread for however long the decode takes
/// @param int deckIdx which registered deck to load onto (see addDeck)
/// @param int fileIdx which file, out of the last reported listing, to load
void FileLoadManager::loadToDeck(int deckIdx, int fileIdx)
{
	if (deckIdx < 0 || deckIdx >= (int)this->decks.size())
		return;

	if (fileIdx < 0 || fileIdx >= (int)this->files.size())
		return;

	Playback::Deck* deck = this->decks[deckIdx];
	std::string fpath = this->files[fileIdx].fpath;

	std::thread(&Playback::Deck::loadTrack, deck, fpath).detach();
}

/// @brief external accessor for the currently selected file's index
int FileLoadManager::getSelectedIdx() const
{
	return this->selectedIdx;
}

/// @brief external setter for the currently selected file's index, clamped into [0, file count
/// - 1] (or pinned to 0 if there are no files) so it can never point past the listing - callers
/// (eg CentralController's up/down arrow callbacks) don't need to know the file count themselves
/// @param int idx the index to select
void FileLoadManager::setSelectedIdx(int idx)
{
	if (this->files.empty())
	{
		this->selectedIdx = 0;
		return;
	}

	this->selectedIdx = std::clamp(idx, 0, (int)this->files.size() - 1);
}

}
