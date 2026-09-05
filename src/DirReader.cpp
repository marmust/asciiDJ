#include <DirReader.hpp>

#include <filesystem>
#include <algorithm>
#include <cctype>

namespace Files
{

/// @brief lists every supported audio file directly inside dir; silently returns an empty vec on
/// any failure (nonexistent dir, permission error, or anything else std::filesystem might throw)
/// @param const std::string& dir the directory to scan (non-recursive, subdirectories ignored)
/// @returns std::vector<FileEntry> the discovered files, empty on failure or if none match
std::vector<FileEntry> DirReader::scanDir(const std::string& dir)
{
	std::vector<FileEntry> found;

	try
	{
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir))
		{
			if (!entry.is_regular_file())
				continue;

			std::string extension = entry.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(),
					[](unsigned char c) { return std::tolower(c); });

			bool isAllowed = std::find(ALLOWED_FILETYPES.begin(), ALLOWED_FILETYPES.end(), extension) != ALLOWED_FILETYPES.end();

			if (!isAllowed)
				continue;

			found.push_back({ entry.path().string(), entry.path().filename().string() });
		}
	}
	catch (...)
	{
		return {};
	}

	return found;
}

/// @brief scans the directory the program is currently running from once, caching the result for
/// listLocalFiles() to hand back - silently ends up with an empty listing on any failure
DirReader::DirReader()
{
	std::error_code ec;
	std::filesystem::path localDir = std::filesystem::current_path(ec);

	if (ec)
		return;

	this->localDir = localDir.string();
	this->files = scanDir(this->localDir);
}

/// @brief plain getter for the listing cached at construction time - no disk I/O
const std::vector<FileEntry>& DirReader::listLocalFiles() const
{
	return this->files;
}

/// @brief plain getter for the dir path scanned at construction time
const std::string& DirReader::getLocalDir() const
{
	return this->localDir;
}

}
