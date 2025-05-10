#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
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

void closeFd(int fd)
{
	if (fd != -1)
	{
		if (close(fd) == -1)
		{
			perror("close");
		}
	}
}

bool setNonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		int err = errno;
		std::cerr << "fcntl F_GETFL error (" << fd << "): " << strerror(err) << std::endl;
		return false;
	}

	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1)
	{
		int err = errno;
		std::cerr << "fcntl F_SETFL error (" << fd << "): " << strerror(err) << std::endl;
		return false;
	}

	return true;
}
