#pragma once
#include <string>
#include <vector>
#include <netdb.h>

std::string trim(const std::string& s);
std::vector<std::string> splitByWhiteSpaces(const std::string &text);
bool isValidAbsolutePath(const std::string &path);
bool isNumber(const std::string &s);
std::string getStraddr(struct addrinfo *p);
void printAddrinfo(const char *host, const char *port, struct addrinfo *ai);
int convertSizeToBytes(const std::string &size);
