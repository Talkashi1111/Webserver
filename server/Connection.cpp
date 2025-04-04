#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include "Connection.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "WebServer.hpp"
#include "Consts.hpp"
#include "Globals.hpp"
#include "StringUtils.hpp"

Connection::Connection(int fd,
					   const std::string &port,
					   const std::string &host,
					   WebServer *ptr) : _fd(fd),
					  					 _webserver(ptr),
										 _port(port),
										 _host(host),
										 _lastActivityTime(time(0)),
										 _serverConfig(NULL),
										 _locationConfig(NULL),
										 _request(ptr->getClientHeaderBufferSize(), ptr->getClientMaxBodySize()),
										 _response(),
										 _clientHeaderBufferSize(ptr->getClientHeaderBufferSize()),
										 _keepAlive(true)

{
}

Connection::Connection(const Connection &connection) : _fd(connection._fd),
													   _webserver(connection._webserver),
													   _port(connection._port),
													   _host(connection._host),
													   _lastActivityTime(connection._lastActivityTime),
													   _serverConfig(connection._serverConfig),
													   _locationConfig(connection._locationConfig),
													   _request(connection._request),
													   _response(connection._response),
													   _clientHeaderBufferSize(connection._clientHeaderBufferSize),
													   _keepAlive(connection._keepAlive)
{
}

Connection &Connection::operator=(const Connection &connection)
{
	if (this != &connection)
	{
		_fd = connection._fd;
		_webserver = connection._webserver;
		_port = connection._port;
		_host = connection._host;
		_lastActivityTime = connection._lastActivityTime;
		_serverConfig = connection._serverConfig;
		_locationConfig = connection._locationConfig;
		_request = connection._request;
		_response = connection._response;
		_clientHeaderBufferSize = connection._clientHeaderBufferSize;
		_keepAlive = connection._keepAlive;
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

const std::string &Connection::getResponse() const
{
	return _response.getResponse();
}

void Connection::eraseResponse(int nbytes)
{
	_response.eraseResponse(nbytes);
}

bool Connection::isKeepAlive() const
{
	return _keepAlive;
}

RequestState Connection::handleClientRecv(const std::string &raw)
{
	try
	{
		_lastActivityTime = time(0);
		_request.parseRequest(raw);

		if (_request.getState() == S_DONE)
		{
			if (DEBUG)
				_request.printRequestDBG();
			// TODO: init Server and Location and generate response
			setServerAndLocation();
			if (_serverConfig == NULL)
				throw std::runtime_error("404");
		}
		return _request.getState();
	}
	catch (const std::exception &e)
	{
		std::string statusCode = e.what();
		_keepAlive = false;
		// TODO: handle error page replacement if configured
		setServerAndLocation();
		if (_serverConfig != NULL)
		{
			std::map<int, std::string>::const_iterator it = _serverConfig->getErrorPages().find(atoi(e.what()));
			if (it != _serverConfig->getErrorPages().end())
			{
				std::string errorPage = it->second;
				std::string errorPagePath = getPath(errorPage);
				_response.setResponseFile(statusCode, errorPagePath);
				return S_ERROR;
			}
		}
		_response.generateErrorResponse(statusCode);
		return S_ERROR;
	}
}

void Connection::reset()
{
	_request = HttpRequest(_clientHeaderBufferSize, _webserver->getClientMaxBodySize());
	_response = HttpResponse();
	_serverConfig = NULL;
	_locationConfig = NULL;
}

void Connection::setServerAndLocation()
{
	_serverConfig = NULL;
	_locationConfig = NULL;
	const std::map<ServerKey, Server *> &servers = _webserver->getServers();
	ServerKey key(_host, _port, _request.getHostName());
	std::map<ServerKey, Server *>::const_iterator it = servers.find(key);
	if (it != servers.end())
	{
		_serverConfig = it->second;
	}
	else // try default server
	{
		key.server_name = kDefaultServerName;
		it = servers.find(key);
		if (it != servers.end())
		{
			_serverConfig = it->second;
		}
		else
		{
			return;
		}
	}
	_locationConfig = _serverConfig->getLocationForURI(_request.getTarget());
}

std::string Connection::getPath(const std::string &path) const
{
	if (_serverConfig == NULL)
		return path;
	std::string root = _serverConfig->getRoot();
	// TODO: verify if we need to add a trailing slash here
	if (!root.empty() && root[root.length() - 1] == '/')
	{
		if (path[0] == '/')
			root += path.substr(1);
		else
			root += path;
	}
	else
	{
		if (path[0] == '/')
			root += path;
		else
			root += "/" + path;
	}
	return root;
}
