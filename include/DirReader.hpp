#pragma once

#include <string>
#include <vector>

namespace Files
{

// extensions DirReader will list - hardcoded to what Deck::loadTrack (miniaudio, built here with
// only its stock WAV/MP3/FLAC decoders, no vorbis/opus extension vendored) can actually decode
inline const std::vector<std::string> ALLOWED_FILETYPES = { ".wav", ".mp3", ".flac" };

struct FileEntry
{
	std::string fpath;
	std::string displayName;
};

class DirReader
{
private:
	std::string localDir;
	std::vector<FileEntry> files;

	// lists every supported audio file directly inside dir (non-recursive, subdirectories
	// ignored); silently returns an empty vec on any failure (nonexistent dir, permission
	// error, or anything else std::filesystem might throw)
	static std::vector<FileEntry> scanDir(const std::string& dir);

public:
	// scans the directory the program is currently running from once, right here, and caches
	// the result - ends up with an empty listing on any failure (nonexistent dir, etc)
	DirReader();

	// plain getter for the listing cached at construction time - no disk I/O
	const std::vector<FileEntry>& listLocalFiles() const;

	// plain getter for the dir path scanned at construction time
	const std::string& getLocalDir() const;
};

}
