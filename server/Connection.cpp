#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <sstream>
#include "Connection.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "WebServer.hpp"
#include "Consts.hpp"
#include "StringUtils.hpp"

Connection::Connection(int fd,
					   const std::string &port,
					   const std::string &host,
					   int clientHeaderBufferSize,
					   WebServer *ptr) : _fd(fd),
					  					 _webserver(ptr),
										 _port(port),
										 _host(host),
										 _lastActivityTime(time(0)),
										 _serverConfig(NULL),
										 _locationConfig(NULL),
										 _request(clientHeaderBufferSize, ptr->getClientMaxBodySize()),
										 _response(),
										 _clientHeaderBufferSize(clientHeaderBufferSize),
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
			_request.printRequestDBG();
			// TODO: init Server and Location and generate response
		}
		return _request.getState();
	}
	catch (const std::exception &e)
	{
		std::string statusCode = e.what();
		std::string statusText = kStatusCodes.find(statusCode)->second;
		std::string body =
			"<html>\n"
			"<head><title>" +
			statusCode + " " + statusText + "</title></head>\n"
											"<body>\n"
											"<center><h1>" +
			statusCode + " " + statusText + "</h1></center>\n"
											"<hr><center>webserver/1.0</center>\n"
											"</body>\n"
											"</html>\n";
		std::ostringstream oss;
		oss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
		oss << "Server: webserver/1.0\r\n";
		oss << "Date: " << getCurrentTime() << "\r\n";
		oss << "Content-Type: text/html\r\n";
		oss << "Content-Length: " << body.length() << "\r\n";
		oss << "Connection: close\r\n";
		oss << "\r\n"
			<< body;
		_response.setResponse(oss.str());
		_keepAlive = false;
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
