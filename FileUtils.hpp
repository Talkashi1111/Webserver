#pragma once
#include <string>

bool isDirectory(const std::string& path);
bool isFile(const std::string& path);
bool readFileToMemory(const std::string& path, std::string& out);
void closeFd(int fd);
bool setNonblocking(int fd);
