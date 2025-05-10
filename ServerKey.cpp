#include "ServerKey.hpp"

ServerKey::ServerKey() : port(""), host(""), server_name("") {}

ServerKey::ServerKey(const std::string &port_, const std::string &host_, const std::string &server_name_)
	: port(port_), host(host_), server_name(server_name_) {}

ServerKey::ServerKey(const ServerKey &other)
	: port(other.port), host(other.host), server_name(other.server_name) {}

ServerKey &ServerKey::operator=(const ServerKey &other)
{
	if (this != &other)
	{
		port = other.port;
		host = other.host;
		server_name = other.server_name;
	}
	return *this;
}

ServerKey::~ServerKey() {}

// Comparison operator for ordering (used in std::map)
bool ServerKey::operator<(const ServerKey &other) const
{
	if (port < other.port)
		return true;
	if (port > other.port)
		return false;
	if (host < other.host)
		return true;
	if (host > other.host)
		return false;
	return server_name < other.server_name;
}
