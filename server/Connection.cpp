#include <unistd.h>
#include <cstdio>
#include <iostream>
#include "Connection.hpp"

Connection::Connection(int fd, const std::string &port, const std::string &host) : _fd(fd),
																				   _port(port),
																				   _host(host),
																				   _lastActivityTime(time(0)),
																				   _serverConfig(NULL),
																				   _locationConfig(NULL)
{
}

Connection::Connection(const Connection &connection) : _fd(connection._fd),
														_port(connection._port),
														_host(connection._host),
														_lastActivityTime(connection._lastActivityTime),
														_serverConfig(connection._serverConfig),
														_locationConfig(connection._locationConfig)
{
}

Connection &Connection::operator=(const Connection &connection)
{
	if (this != &connection)
	{
		_fd = connection._fd;
		_port = connection._port;
		_host = connection._host;
		_lastActivityTime = connection._lastActivityTime;
		_serverConfig = connection._serverConfig;
		_locationConfig = connection._locationConfig;
	}
	return *this;
}

Connection::~Connection()
{
}

int Connection::getFd() const
{
	return _fd;
}

void Connection::setFd(int fd)
{
	_fd = fd;
}

const std::string &Connection::getPort() const
{
	return _port;
}

void Connection::setPort(const std::string &port)
{
	_port = port;
}

const std::string &Connection::getHost() const
{
	return _host;
}

void Connection::setHost(const std::string &host)
{
	_host = host;
}

time_t Connection::getLastActivityTime() const
{
	return _lastActivityTime;
}

void Connection::setLastActivityTime(time_t lastActivityTime)
{
	_lastActivityTime = lastActivityTime;
}

void Connection::updateActivityTime()
{
	_lastActivityTime = time(0);
}

Server *Connection::getServerConfig() const
{
	return _serverConfig;
}

void Connection::setServerConfig(Server *serverConfig)
{
	_serverConfig = serverConfig;
}

Location *Connection::getLocationConfig() const
{
	return _locationConfig;
}

void Connection::setLocationConfig(Location *locationConfig)
{
	_locationConfig = locationConfig;
}

