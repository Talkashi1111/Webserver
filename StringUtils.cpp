#include <ctime>
#include <sstream>
#include <iostream>
#include <cstdlib> // for atoi
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

/**
 * Splits a string into words by whitespace while preserving quoted text as single words.
 *
 * @param text The input string to split
 * @return Vector of strings containing the split words
 *
 * Notes:
 * - Preserves quoted strings as single words including the quotes
 * - Ignores whitespace inside quoted strings
 * - Treats both spaces and tabs as word separators
 * - Empty quoted strings are preserved
 * - Multiple consecutive whitespaces are treated as one separator
 */
std::vector<std::string> splitByWhiteSpaces(const std::string &text)
{
	std::vector<std::string> words;
	std::string word;
	bool inQuotes = false;

	for (size_t i = 0; i < text.length(); ++i)
	{
		char c = text[i];

		if (c == '"')
		{
			inQuotes = !inQuotes;
			word += c;
		}
		else if ((c == ' ' || c == '\t') && !inQuotes)
		{
			if (!word.empty())
			{
				words.push_back(word);
				word.clear();
			}
		}
		else
		{
			word += c;
		}
	}

	// Add last word if exists
	if (!word.empty())
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
int convertSizeToBytes(const std::string &size)
{
	size_t len = size.length();
	int multiplier = 1;
	std::string number_str;

	if (size[len - 1] == 'k' || size[len - 1] == 'K')
	{
		multiplier = 1024;
		number_str = size.substr(0, len - 1);
	}
	else if (size[len - 1] == 'm' || size[len - 1] == 'M')
	{
		multiplier = 1024 * 1024;
		number_str = size.substr(0, len - 1);
	}
	else
	{
		number_str = size;
	}

	return atoi(number_str.c_str()) * multiplier;
}

std::string getCurrentTime()
{
	char date[100];
	time_t now = time(0);
	struct tm tm = *gmtime(&now);

	strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", &tm);

	return std::string(date);
}

bool validHttpRequestChar(char c)
{
	const std::string allowedSymbols = "!#$%&'*+-.^_`|~";
	if ((c >= 'a' && c <= 'z') || allowedSymbols.find(c) != std::string::npos ||
		(c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
		return true;
	return false;
}

std::string trimFromEnd(const std::string &str)
{
	size_t end = str.length();
	while (end > 0 && (str[end - 1] == ' ' || str[end - 1] == '\t'))
	{
		--end;
	}
	if (end == str.length())
		return str;
	if (end == 0)
		return "";
	return str.substr(0, end);
}

// Helper function to convert numbers to strings (replacement for std::to_string)
std::string numberToString(size_t value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
}
