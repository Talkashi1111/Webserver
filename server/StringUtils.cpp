#include <sstream>
#include <iostream>
#include <arpa/inet.h>
#include "StringUtils.hpp"

std::string trim(const std::string &s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

std::vector<std::string> splitByWhiteSpaces(const std::string &text)
{
	std::vector<std::string> words;
	std::string word;
	std::istringstream iss(text);
	while (iss >> word)
	{
		words.push_back(word);
	}
	return words;
}

bool isValidAbsolutePath(const std::string &path)
{
	// Check if path is empty or doesn't start with '/'
	if (path.empty() || path[0] != '/')
		return false;

	// Special case for root path "/"
	if (path == "/")
		return true;

	// Check for double slashes "//"
	if (path.find("//") != std::string::npos)
		return false;

	// Check for current directory "." or parent directory ".." references
	if (path.find("/./") != std::string::npos ||
		path.find("/../") != std::string::npos ||
		path.find("/.") == path.length() - 2 ||
		path.find("/..") == path.length() - 3)
		return false;

	// Check for invalid characters in the path
	for (size_t i = 0; i < path.length(); ++i)
	{
		char c = path[i];
		// Allow alphanumeric, '/', '-', '_', '.', '~'
		if (!isalnum(c) && c != '/' && c != '-' && c != '_' &&
			c != '.' && c != '~')
			return false;
	}

	return true;
}

bool isNumber(const std::string &s)
{
	for (size_t i = 0; i < s.length(); ++i)
	{
		if (!isdigit(s[i]))
			return false;
	}
	return true;
}

std::string getStraddr(struct addrinfo *p)
{
	char ipstr[INET6_ADDRSTRLEN];
	void *addr;

	// Get the pointer to the address itself,
	// different fields in IPv4 and IPv6:
	if (p->ai_family == AF_INET) // IPv4
	{
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		addr = &(ipv4->sin_addr);
	}
	else // IPv6
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
		addr = &(ipv6->sin6_addr);
	}

	// Convert the IP to a string and print it:
	inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
	return std::string(ipstr);
}

void printAddrinfo(const char *host, const char *port, struct addrinfo *ai)
{
	struct addrinfo *p;

	std::cout << "addrinfo for " << host << ":" << port << std::endl;
	for (p = ai; p != NULL; p = p->ai_next)
	{
		std::string ipver = "IPv4";
		if (p->ai_family == AF_INET6)
			ipver = "IPv6";
		std::string straddr = getStraddr(p);
		std::cout << "\t" << ipver << ": " << straddr << std::endl;
	}
}
