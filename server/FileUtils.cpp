#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include "FileUtils.hpp"

// Return true if the path is a directory
bool isDirectory(const std::string &path)
{
	struct stat s;
	return (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode));
}

// Return true if the path is a regular file
bool isFile(const std::string &path)
{
	struct stat s;
	return (stat(path.c_str(), &s) == 0 && S_ISREG(s.st_mode));
}

// Try to read the whole file into a string
bool readFileToMemory(const std::string &path, std::string &out)
{
	std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open())
		return false;

	std::ostringstream content;
	content << file.rdbuf();
	out = content.str();
	return true;
}
