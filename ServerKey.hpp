#pragma once

#include <string>

class ServerKey
{
public:
	ServerKey();
	ServerKey(const std::string &port_, const std::string &host_, const std::string &server_name_);
	ServerKey(const ServerKey &other);
	ServerKey &operator=(const ServerKey &other);
	~ServerKey();

	// Comparison operator for use in associative containers (e.g., std::map)
	bool operator<(const ServerKey &other) const;

	std::string port;
	std::string host;
	std::string server_name;
};
