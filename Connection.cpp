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
					   const std::string &remotePort,
					   const std::string &remoteHost,
					   WebServer *ptr) : _fd(fd),
										 _webserver(ptr),
										 _port(port),
										 _host(host),
										 _remotePort(remotePort),
										 _remoteHost(remoteHost),
										 _lastActivityTime(time(0)),
										 _serverConfig(NULL),
										 _locationConfig(NULL),
										 _cgi(),
										 _request(ptr->getClientHeaderBufferSize(), ptr->getClientMaxBodySize()),
										 _response(),
										 _keepAlive(kDefaultKeepAlive)

{
}

Connection::Connection(const Connection &connection) : _fd(connection._fd),
													   _webserver(connection._webserver),
													   _port(connection._port),
													   _host(connection._host),
													   _remotePort(connection._remotePort),
													   _remoteHost(connection._remoteHost),
													   _lastActivityTime(connection._lastActivityTime),
													   _serverConfig(connection._serverConfig),
													   _locationConfig(connection._locationConfig),
													   _cgi(connection._cgi),
													   _request(connection._request),
													   _response(connection._response),
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
		_remotePort = connection._remotePort;
		_remoteHost = connection._remoteHost;
		_lastActivityTime = connection._lastActivityTime;
		_serverConfig = connection._serverConfig;
		_locationConfig = connection._locationConfig;
		_cgi = connection._cgi;
		_request = connection._request;
		_response = connection._response;
		_keepAlive = connection._keepAlive;
	}
	return *this;
}

Connection::~Connection()
{
	_cgi.reset();
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

const std::string &Connection::getRemotePort() const
{
	return _remotePort;
}

void Connection::setRemotePort(const std::string &remotePort)
{
	_remotePort = remotePort;
}

const std::string &Connection::getRemoteHost() const
{
	return _remoteHost;
}

void Connection::setRemoteHost(const std::string &remoteHost)
{
	_remoteHost = remoteHost;
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

const std::string &Connection::getResponse() const
{
	return _response.getResponse();
}

void Connection::eraseResponse(int nbytes)
{
	_response.eraseResponse(nbytes);
}

pid_t Connection::getCgiPid() const
{
	return _cgi.getPid();
}

void Connection::setCgiPid(pid_t pid)
{
	_cgi.setPid(pid);
}

int Connection::getCgiInFd() const
{
	return _cgi.getInFd();
}

void Connection::setCgiInFd(int fd)
{
	_cgi.setInFd(fd);
}

int Connection::getCgiOutFd() const
{
	return _cgi.getOutFd();
}

void Connection::setCgiOutFd(int fd)
{
	_cgi.setOutFd(fd);
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
		updateActivityTime();
		_request.parseRequest(raw);

		if (_request.getState() == S_DONE)
		{
			if (DEBUG)
				_request.printRequestDBG();
			setServerAndLocation();
			if (_serverConfig == NULL)
				throw std::runtime_error("404");
			this->_keepAlive = _request.isKeepAlive();
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
		if (DEBUG)
			_request.printRequestDBG();
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

bool Connection::processCgiHeaders(const std::string &cgiData, std::string &statusCode,
								   std::map<std::string, std::string> &cgiHeaders,
								   std::string &body)
{
	// Look for the end of headers
	size_t headerEnd = cgiData.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
	{
		statusCode = "502"; // Bad Gateway
		return false;		// Headers not complete
	}

	// Split into headers and body
	std::string headers = cgiData.substr(0, headerEnd);
	body = cgiData.substr(headerEnd + 4); // +4 to skip "\r\n\r\n"

	// Process headers
	std::istringstream headerStream(headers);
	std::string line;

	statusCode = "200"; // Default status code
	bool hasContentType = false;

	while (std::getline(headerStream, line) && !line.empty())
	{
		if (line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);

		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos)
		{
			std::string name = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 1);

			// Trim leading spaces from value
			size_t firstNonSpace = value.find_first_not_of(" \t");
			if (firstNonSpace != std::string::npos)
				value = value.substr(firstNonSpace);

			// Convert header name to lowercase for case-insensitive comparison
			std::string nameLower = name;
			for (size_t i = 0; i < nameLower.length(); ++i)
				nameLower[i] = std::tolower(nameLower[i]);

			// Check for special headers
			if (nameLower == "status")
			{
				// Extract status code from value (e.g., "200 OK")
				size_t spacePos = value.find(' ');
				if (spacePos != std::string::npos)
					statusCode = value.substr(0, spacePos);
				else
					statusCode = value;
			}
			else if (nameLower == "content-length")
			{
				// don't add it, we will add it later
			}
			else
			{
				if (nameLower == "content-type")
					hasContentType = true;

				cgiHeaders[name] = value;
			}
		}
	}

	// Check for required Content-Type header
	if (!hasContentType)
	{
		statusCode = "502"; // Bad Gateway
		return false;
	}

	return true;
}

std::string Connection::buildHttpResponse(const std::string &statusCode,
										  const std::map<std::string, std::string> &headers,
										  const std::string &body)
{
	std::ostringstream oss;

	// Build the HTTP response
	oss << "HTTP/1.1 " << statusCode << " " << kStatusCodes.find(statusCode)->second << "\r\n";
	oss << "Server: webserver/1.0\r\n";
	oss << "Date: " << getCurrentTime() << "\r\n";

	// Add CGI headers
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		 it != headers.end(); ++it)
	{
		oss << it->first << ": " << it->second << "\r\n";
	}

	oss << "Content-Length: " << body.length() << "\r\n";

	// Add Connection header
	oss << "Connection: " << (_keepAlive ? "keep-alive" : "close") << "\r\n";

	// End headers
	oss << "\r\n";

	// Add body
	oss << body;

	return oss.str();
}

RequestState Connection::finalizeCgiRecv(int fd)
{
	updateActivityTime();

		// EOF reached, CGI has finished sending data
		if (DEBUG)
			std::cout << "CGI process completed output on fd " << fd << std::endl;

		// If no data was ever received, generate an error
		if (_response.getResponse().empty())
		{
			_keepAlive = false;
			_response.generateErrorResponse("502"); // Bad Gateway
			return S_ERROR;
		}

		// Now that we have all the data, process the CGI response
		std::string responseData = _response.getResponse();
		std::string statusCode;
		std::map<std::string, std::string> cgiHeaders;
		std::string body;

		// Process headers if we have a complete response
		if (!processCgiHeaders(responseData, statusCode, cgiHeaders, body))
		{
			// No headers section found or missing Content-Type
			_keepAlive = false;
			_response.generateErrorResponse("502");
			return S_ERROR;
		}
		_response.setResponse(buildHttpResponse(statusCode, cgiHeaders, body));

		return S_DONE;
}

RequestState Connection::handleCgiRecv(int fd)
{
	char buf[kMaxBuff]; // Buffer for CGI output data
	int nbytes;

	// Read data from the pipe
	nbytes = read(fd, buf, kMaxBuff - 1);
	if (nbytes < 0)
	{
		// Error reading from CGI pipe
		perror("read from CGI pipe");
		_keepAlive = false;
		_response.generateErrorResponse("500"); // Internal Server Error
		return S_ERROR;
	}
	else if (nbytes == 0)
	{
		return finalizeCgiRecv(fd);
	}
	else // We got some data from the CGI process
	{
		updateActivityTime();

		// Check if adding the new data would exceed the client_max_body_size limit
		if (_response.getResponse().length() + nbytes > _webserver->getClientMaxBodySize())
		{
			_keepAlive = false;
			_response.generateErrorResponse("413"); // Request Entity Too Large
			return S_ERROR;
		}

		buf[nbytes] = '\0';
		_response.appendResponse(std::string(buf, nbytes));

		return S_CGI_PROCESSING; // Continue processing
	}
}

RequestState Connection::handleCgiSend(int fd)
{
	updateActivityTime();

	size_t cgiBodySentBytes = _cgi.getCgiBodySentBytes();
	const std::string &body = _request.getBody();
	if (body.empty() || _request.getMethod() != "POST")
	{
		return S_DONE;
	}

	size_t remainingBytes = body.size() - cgiBodySentBytes;
	ssize_t bytesSent = write(fd,
							  body.c_str() + cgiBodySentBytes,
							  remainingBytes);
	if (bytesSent < 0)
	{
		perror("write to CGI pipe");
		_keepAlive = false;
		_response.generateErrorResponse("500"); // Internal Server Error
		return S_ERROR;
	}

	cgiBodySentBytes += bytesSent;
	_cgi.setCgiBodySentBytes(cgiBodySentBytes);
	if (cgiBodySentBytes >= body.size())
	{
		if (DEBUG)
			std::cout << "All CGI input data sent on fd " << fd << std::endl;
		return S_DONE;
	}

	// If we get here, we sent some data but not all
	// We'll continue sending in the next iteration
	return S_CGI_PROCESSING;
}

void Connection::reset()
{
	_request = HttpRequest(_webserver->getClientHeaderBufferSize(), _webserver->getClientMaxBodySize());
	_response = HttpResponse();
	_serverConfig = NULL;
	_locationConfig = NULL;
	_cgi.reset();
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
	ServerKey key(_port, _host, _request.getHostName());
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

std::string Connection::getCgiPath(const std::string &path) const
{
	size_t dotPos = path.find_last_of('.');
	if (dotPos == std::string::npos)
		return "";

	std::string ext = path.substr(dotPos);
	if (ext.empty())
		return "";

	std::map<std::string, std::string>::const_iterator it = _serverConfig->getCgiBin().find(ext);
	if (it != _serverConfig->getCgiBin().end())
	{
		return it->second;
	}
	return "";
}

/* std::string Connection::generateAutoIndex(const std::string &path) const
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
} */

std::string Connection::generateAutoIndex(const std::string& path, const std::string &target) const
{
	std::ostringstream oss;
	oss << "<!DOCTYPE html>\n<html>\n<head>\n"
		<< "  <meta charset=\"utf-8\">\n"
		<< "  <title>Index of " << path << "</title>\n"
		<< "  <style>\n"
		<< "    /* shiny pink auto-index */\n"
		<< "    body {\n"
		<< "      background-color: #fff0f5; /* Light pink background */\n"
		<< "      font-family: Arial, sans-serif;\n"
		<< "      margin: 0;\n"
		<< "      padding: 20px;\n"
		<< "      display: flex;\n"
		<< "      flex-direction: column;\n"
		<< "      align-items: center;\n"
		<< "      min-height: 100vh;\n"
		<< "    }\n"
		<< "    h1.autoindex-title {\n"
		<< "      display: inline-block;\n"
		<< "      padding: 10px 20px;\n"
		<< "      border-radius: 8px;\n"
		<< "      color: #fff;\n"
		<< "      text-align: center;\n"
		<< "      margin: 20px 0;\n"
		<< "      background: linear-gradient(135deg, #ff9ad9 0%, #ff4cbe 50%, #ff1493 100%);\n"
		<< "      box-shadow: 0 0 12px rgba(255,20,147,.7);\n"
		<< "      text-shadow: 0 2px 4px rgba(0,0,0,0.2);\n"
		<< "    }\n"
		<< "    hr {\n"
		<< "      width: 80%;\n"
		<< "      border: none;\n"
		<< "      height: 1px;\n"
		<< "      background: rgba(255,20,147,0.3);\n"
		<< "      margin: 20px 0;\n"
		<< "    }\n"
		<< "    ul.autoindex {\n"
		<< "      list-style: none;\n"
		<< "      margin: 0;\n"
		<< "      padding: 0;\n"
		<< "      text-align: center;\n"
		<< "      width: 80%;\n"
		<< "      max-width: 600px;\n"
		<< "    }\n"
		<< "    ul.autoindex li {\n"
		<< "      margin: 8px 0;\n"
		<< "    }\n"
		<< "    ul.autoindex a {\n"
		<< "      display: inline-block;\n"
		<< "      padding: 6px 12px;\n"
		<< "      border-radius: 6px;\n"
		<< "      font-weight: 700;\n"
		<< "      color: #fff;\n"
		<< "      text-decoration: none;\n"
		<< "      background: linear-gradient(135deg, #ff9ad9 0%, #ff4cbe 50%, #ff1493 100%);\n"
		<< "      box-shadow: 0 0 8px rgba(255,20,147,.6);\n"
		<< "      transition: transform .3s, box-shadow .3s;\n"
		<< "      min-width: 150px;\n"
		<< "    }\n"
		<< "    ul.autoindex a:hover {\n"
		<< "      transform: translateY(-3px) scale(1.06);\n"
		<< "      box-shadow: 0 0 12px rgba(255,20,147,.85), 0 0 22px rgba(255,20,147,.65);\n"
		<< "    }\n"
		<< "    .counter {\n"
		<< "      background: rgba(255,20,147,0.1);\n"
		<< "      border-radius: 8px;\n"
		<< "      padding: 8px 16px;\n"
		<< "      margin-top: 20px;\n"
		<< "      font-weight: bold;\n"
		<< "      color: #ff1493;\n"
		<< "      box-shadow: 0 0 5px rgba(255,20,147,.3);\n"
		<< "      text-align: center;\n"
		<< "    }\n"
		<< "  </style>\n"
		<< "</head>\n<body>\n"
		<< "  <h1 class=\"autoindex-title\">Index of " << path << "</h1>\n"
		<< "  <hr>\n"
		<< "  <ul class=\"autoindex\">\n";

	DIR* dir = opendir(path.c_str());
	if (!dir) return "";

	int entryCount = 0;
	for (dirent* entry; (entry = readdir(dir));) {
		if (entry->d_name[0] == '.') continue; // Skip hidden files
		std::string name = entry->d_name;
		if (isDirectory(path + '/' + name)) name += '/';
		oss << "    <li><a href=\"" << target + '/' + name << "\">" << name << "</a></li>\n";
		entryCount++;
	}
	closedir(dir);

	oss << "  </ul>\n"
		<< "  <hr>\n";

	if (entryCount == 0)
		oss << "  <div class=\"counter\">No entries found</div>\n";

	oss << "</body>\n</html>\n";
	return oss.str();
}

void Connection::generateResponse()
{
	std::string body;
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
	if (isDirectory(fullPath))
	{
		if (_locationConfig->getAutoindex())
		{
			std::string autoindex = generateAutoIndex(fullPath, _request.getTarget());
			std::ostringstream oss;
			oss << "HTTP/1.1 200 OK\r\n";
			oss << "Server: webserver/1.0\r\n";
			oss << "Date: " << getCurrentTime() << "\r\n";
			setContentType(fullPath, oss);
			oss << "Content-Length: " << autoindex.length() << "\r\n";
			oss << "Content-Type: text/html\r\n";
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
		std::string cgiPath = getCgiPath(fullPath);
		if (!cgiPath.empty())
		{
			_cgi.start(_request, cgiPath, fullPath, _port, _remoteHost, _locationConfig->getUploadDirectory());
			_request.setState(S_CGI_PROCESSING);
			return;
		}
		else if (!readFileToMemory(fullPath, body))
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
