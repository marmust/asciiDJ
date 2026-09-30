#pragma once

#include <string>
#include <vector>

namespace Files
{

// extensions DirReader lists, limited to the decoders miniaudio is built with here
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

	// lists supported audio files directly inside dir (non-recursive), empty on any failure
	static std::vector<FileEntry> scanDir(const std::string& dir);

public:
	// scans the working directory once and caches the result, empty on any failure
	DirReader();

	// plain getter for the listing cached at construction time - no disk I/O
	const std::vector<FileEntry>& listLocalFiles() const;

	// plain getter for the dir path scanned at construction time
	const std::string& getLocalDir() const;
};

}
