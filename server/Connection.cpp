#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <dirent.h> //for readdir
#include "Connection.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "WebServer.hpp"
#include "Consts.hpp"
#include "Globals.hpp"
#include "StringUtils.hpp"
#include "FileUtils.hpp"
#include "CGI.hpp"

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
										 _keepAlive(kDefaultKeepAlive)

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

bool Connection::isAllowdMethod(const std::string &method, const std::map<std::string, bool> methods) const
{
	std::map<std::string, bool>::const_iterator it = methods.find(method);
	if (it != methods.end())
		return it->second;
	return false;
}

RequestState Connection::handleClientRecv(const std::string &raw)
{
	try
	{
		_lastActivityTime = time(0);
		_request.parseRequest(raw);
		std::string ext = "";

		if (_request.getState() == S_DONE)
		{
			if (DEBUG)
				_request.printRequestDBG();
			setServerAndLocation();
			if (_serverConfig == NULL)
				throw std::runtime_error("404");
			this->_keepAlive = _request.isKeepAlive();
			// TODO: generate response
			if (_serverConfig->isReturnDirectiveSet())
			{
				std::pair<std::string, std::string> returnDirective = _serverConfig->getReturnDirective();
				generateReturnDirectiveResponse(returnDirective.first, returnDirective.second);
				return _request.getState();
			}
			else if (_locationConfig == NULL)
				throw std::runtime_error("404");
			if (!isAllowdMethod(_request.getMethod(), _locationConfig->getAllowedMethods()))
				throw std::runtime_error("405");
			else if (_locationConfig->isReturnDirectiveSet()) // location config is not NULL, meaning we have a location block
			{
				std::pair<std::string, std::string> returnDirective = _locationConfig->getReturnDirective();
				generateReturnDirectiveResponse(returnDirective.first, returnDirective.second);
				return _request.getState();
			}
			else
				generateResponse();
		}
		return _request.getState();
	}
	catch (const std::exception &e)
	{
		std::string statusCode = e.what();
		_keepAlive = false;
		setServerAndLocation();
		if (_serverConfig != NULL)
		{
			std::map<int, std::string>::const_iterator it = _serverConfig->getErrorPages().find(atoi(e.what()));
			if (it != _serverConfig->getErrorPages().end())
			{
				std::string errorPage = it->second;
				std::string errorPagePath = resolvePath(_serverConfig->getRoot(), errorPage);
				_response.generateErrorResponseFile(statusCode, errorPagePath);
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

/**
 * Sets the server and location configuration for the current request.
 * Attempts to match a server block in the following order:
 *
 * 1. Exact match: <address>:<port> "server_name"
 *    Example: 127.0.0.1:8081 "example.com"
 *
 * 2. Address match with default server name: <address>:<port> ""
 *    Example: 127.0.0.1:8081 ""
 *
 * 3. Address match with default host: 0.0.0.0:<port> "server_name"
 *    Example: 0.0.0.0:8081 "example.com"
 *
 * 4. Default server (fallback): 0.0.0.0:<port> ""
 *    Example: 0.0.0.0:8081 ""
 *
 * After finding a matching server block, searches for a matching
 * location block based on the request URI.
 */
void Connection::setServerAndLocation()
{
	// Check if the server is already set
	// we don't check if the location is already set
	// because the location cannot be set without the server
	if (_serverConfig != NULL)
		return;
	const std::map<ServerKey, Server *> &servers = _webserver->getServers();
	ServerKey key(_host, _port, _request.getHostName());
	std::map<ServerKey, Server *>::const_iterator it = servers.find(key);
	if (it != servers.end())
	{
		_serverConfig = it->second;
	}
	if (_serverConfig == NULL)
	{
		key.server_name = kDefaultServerName;
		it = servers.find(key);
		if (it != servers.end())
		{
			_serverConfig = it->second;
		}
	}
	if (_serverConfig == NULL)
	{
		key.host = kDefaultHost;
		key.server_name = _request.getHostName();
		it = servers.find(key);
		if (it != servers.end())
		{
			_serverConfig = it->second;
		}
	}
	if (_serverConfig == NULL)
	{
		key.host = kDefaultHost;
		key.server_name = kDefaultServerName;
		it = servers.find(key);
		if (it != servers.end())
		{
			_serverConfig = it->second;
		}
	}
	if (_serverConfig == NULL)
	{
		return;
	}
	_locationConfig = _serverConfig->getLocationForURI(_request.getTarget());
}

std::string Connection::resolvePath(const std::string &root, const std::string &path) const
{
	std::string absPath = root;
	if (!root.empty() && root[root.length() - 1] == '/')
	{
		if (path[0] == '/')
			absPath += path.substr(1);
		else
			absPath += path;
	}
	else
	{
		if (path[0] == '/')
			absPath += path;
		else
			absPath += "/" + path;
	}
	return absPath;
}

void Connection::generateReturnDirectiveResponse(const std::string &status, const std::string &redirectPath)
{
	std::string text = redirectPath;
	if (text[0] == '"')
		text = text.substr(1, text.length() - 2);

	std::string statusText = kStatusCodes.find(status)->second;
	if (status == "301" || status == "302" || status == "303" || status == "307" || status == "308")
	{
		std::string body =
			"<html>\n"
			"<head><title>" +
			status + " " + statusText + "</title></head>\n"
										"<body>\n"
										"<center><h1>" +
			status + " " + statusText + "</h1></center>\n"
										"<hr><center>webserver/1.0</center>\n"
										"</body>\n"
										"</html>\n";
		std::ostringstream oss;
		oss << "HTTP/1.1 " << status << " " << statusText << "\r\n";
		oss << "Server: webserver/1.0\r\n";
		oss << "Date: " << getCurrentTime() << "\r\n";
		oss << "Content-Type: text/html\r\n";
		oss << "Content-Length: " << body.length() << "\r\n";
		oss << "Connection: " << (_keepAlive ? "keep-alive" : "close") << "\r\n";
		oss << "Location: " << text << "\r\n";
		oss << "\r\n"
			<< body;
		_response.setResponse(oss.str());
	}
	else
	{
		std::string body = text;
		std::ostringstream oss;
		oss << "HTTP/1.1 " << status << " " << statusText << "\r\n";
		oss << "Server: webserver/1.0\r\n";
		oss << "Date: " << getCurrentTime() << "\r\n";
		oss << "Content-Type: application/octet-stream\r\n";
		oss << "Content-Length: " << body.length() << "\r\n";
		oss << "Connection: " << (_keepAlive ? "keep-alive" : "close") << "\r\n";
		oss << "\r\n"
			<< body;
		_response.setResponse(oss.str());
	}
}

// TODO: finish
bool Connection::isCGI(std::string path, std::string &ext) const
{
	std::string::reverse_iterator it = path.rbegin();
	for (; it != path.rend(); ++it)
	{
		if (*it == '.')
			break;
	}
	if (it == path.rend())
		return false;
	size_t dotPos = path.length() - (it - path.rbegin()) - 1;
	ext = path.substr(dotPos);
	std::map<std::string, std::string>::const_iterator it_cgi;
	for (it_cgi = _serverConfig->getCgiBin().begin(); it_cgi != _serverConfig->getCgiBin().end(); ++it_cgi)
	{
		if (it_cgi->first == ext)
			return true;
	}

	return false;
}

std::string Connection::generateAutoIndex(const std::string &path) const
{
	std::ostringstream oss;
	oss << "<html>\n"
		<< "<head><title>Index of " << path << "</title></head>\n"
		<< "<body>\n"
		<< "<h1>Index of " << path << "</h1>\n"
		<< "<hr>\n"
		<< "<ul>\n";

	DIR *dir = opendir(path.c_str());
	if (dir == NULL)
		return "";

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_name[0] == '.')
			continue;
		std::string name = entry->d_name;
		std::string fullPath = path + "/" + name;
		if (isDirectory(fullPath))
			name += "/";
		oss << "<li><a href=\"" << name << "\">" << name << "</a></li>\n";
	}
	closedir(dir);

	oss << "</ul>\n"
		<< "<hr>\n"
		<< "</body>\n"
		<< "</html>\n";
	return oss.str();
}

void Connection::generateResponse()
{
	std::string fullPath(resolvePath(_locationConfig->getRoot(), _request.getTarget()));
	if (fullPath[fullPath.length() - 1] == '/')
	{
		std::set<std::string> indexes = _locationConfig->getIndex();
		for (std::set<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
		{
			std::string indexFile = *it;
			if (isFile(fullPath + indexFile))
			{
				fullPath += indexFile;
				break;
			}
		}
	}
	else
	{
		std::string ext = "cgi";
		if (!ext.empty() && isCGI(fullPath, ext))
		{
			if (_serverConfig->getCgiBin().find(ext) != _serverConfig->getCgiBin().end())
			{
				std::string cgiPath = _serverConfig->getCgiBin().find(ext)->second;
				CGI cgi(cgiPath, _webserver);
				std::string body = cgi.execute(fullPath, _request);
				std::ostringstream oss;
				oss << "HTTP/1.1 200 OK\r\n";
				oss << "Server: webserver/1.0\r\n";
				oss << "Date: " << getCurrentTime() << "\r\n";
				setContentType(fullPath, oss);
				oss << "Content-Length: " << body.length() << "\r\n";
				oss << "Connection: " << (_keepAlive ? "keep-alive" : "close") << "\r\n";
				oss << "\r\n"
					<< body;
				_response.setResponse(oss.str());
				return;
			}
		}
	}

	std::string body;
	if (isDirectory(fullPath))
	{
		if (_locationConfig->getAutoindex())
		{
			std::string autoindex = generateAutoIndex(fullPath);
			std::ostringstream oss;
			oss << "HTTP/1.1 200 OK\r\n";
			oss << "Server: webserver/1.0\r\n";
			oss << "Date: " << getCurrentTime() << "\r\n";
			setContentType(fullPath, oss);
			oss << "Content-Length: " << autoindex.length() << "\r\n";
			oss << "Connection: " << (_keepAlive ? "keep-alive" : "close") << "\r\n";
			oss << "\r\n"
				<< autoindex;
			_response.setResponse(oss.str());
			return;
		}
		else
		{
			std::string uri = _request.getTarget();
			if (uri[uri.length() - 1] != '/')
				uri += '/';
			std::string redir = "http://" + _request.getHostName() + uri;
			generateReturnDirectiveResponse("301", redir);
			return;
		}
	}
	else if (isFile(fullPath))
	{
		if (!readFileToMemory(fullPath, body))
		{
			throw std::runtime_error("403");
		}
		else
		{
			std::ostringstream oss;
			oss << "HTTP/1.1 200 OK\r\n";
			oss << "Server: webserver/1.0\r\n";
			oss << "Date: " << getCurrentTime() << "\r\n";
			setContentType(fullPath, oss);
			oss << "Content-Length: " << body.length() << "\r\n";
			oss << "Connection: " << (_keepAlive ? "keep-alive" : "close") << "\r\n";
			oss << "\r\n"
				<< body;
			_response.setResponse(oss.str());
		}
	}
	else
	{
		throw std::runtime_error("404");
	}
}

void Connection::setContentType(const std::string &path, std::ostringstream &oss)
{
	if (path.find(".html") != std::string::npos)
		oss << "Content-Type: text/html\r\n";
	else if (path.find(".css") != std::string::npos)
		oss << "Content-Type: text/css\r\n";
	else if (path.find(".js") != std::string::npos)
		oss << "Content-Type: application/javascript\r\n";
	else if (path.find(".png") != std::string::npos)
		oss << "Content-Type: image/png\r\n";
	else if (path.find(".jpg") != std::string::npos)
		oss << "Content-Type: image/jpeg\r\n";
	else if (path.find(".gif") != std::string::npos)
		oss << "Content-Type: image/gif\r\n";
	else if (path.find(".ico") != std::string::npos)
		oss << "Content-Type: image/x-icon\r\n";
	else if (path.find(".txt") != std::string::npos)
		oss << "Content-Type: text/plain\r\n";
	else if (path.find(".pdf") != std::string::npos)
		oss << "Content-Type: application/pdf\r\n";
	else if (path.find(".zip") != std::string::npos)
		oss << "Content-Type: application/zip\r\n";
	else
		oss << "Content-Type: application/octet-stream\r\n";
}
