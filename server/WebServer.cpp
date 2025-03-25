// Essential system includes
#include <sys/epoll.h>	// for epoll functions
#include <sys/socket.h> // for socket functions
#include <netdb.h>		// for getaddrinfo
#include <arpa/inet.h>	// for inet_ntop
#include <unistd.h>		// for close
#include <fcntl.h>		// for fcntl
#include <errno.h>		// for errno
#include <cstring>		// for strerror
#include <cstdlib>		// for atoi

// Standard C++ includes
#include <string>	 // for string operations
#include <map>		 // for map containers
#include <set>		 // for set containers
#include <iostream>	 // for cout/cerr
#include <fstream>	 // for file operations
#include <stdexcept> // for exceptions
#include <sstream>	 // for string stream

// Project includes
#include "WebServer.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include "Connection.hpp"
#include "StringUtils.hpp"
#include "Globals.hpp"

WebServer::WebServer(const std::string &filename) : _fileName(filename),
													_epfd(-1),
													_listeners(),
													_clientTimeout(kDefaultClientTimeout),
													_clientTimeoutSet(false),
													_clientHeaderBufferSize(kDefaultClientHeaderBufferSize),
													_clientHeaderBufferSizeSet(false)
{
	if (filename.empty())
	{
		throw std::invalid_argument("Empty filename");
	}
	else if (filename.length() < 5 || filename.find(".conf") != filename.length() - 5)
	{
		throw std::invalid_argument("Invalid file extension");
	}
	this->parseConfig();
}

// TODO: fix the copy logic in Server, LocationTrie, LocationTrieNode
WebServer::WebServer(const WebServer &other) : _fileName(other._fileName),
											   _epfd(-1),
											   _listeners(other._listeners),
											   _clientTimeout(other._clientTimeout),
											   _clientTimeoutSet(other._clientTimeoutSet),
											   _clientHeaderBufferSize(other._clientHeaderBufferSize),
											   _clientHeaderBufferSizeSet(other._clientHeaderBufferSizeSet)
{
	// Deep copy each server and store in _servers map
	for (std::map<ServerKey, Server *>::const_iterator it = other._servers.begin();
		 it != other._servers.end(); ++it)
	{
		_servers[it->first] = new Server(*it->second);
	}
}

// TODO: fix the copy logic in Server, LocationTrie, LocationTrieNode
WebServer &WebServer::operator=(const WebServer &other)
{
	if (this != &other)
	{
		// Clean up existing resources
		cleanupServers();

		// Copy basic members
		_fileName = other._fileName;
		_epfd = -1; // Don't copy _epfd, create new one when needed
		_listeners = other._listeners;
		_clientTimeout = other._clientTimeout;
		_clientTimeoutSet = other._clientTimeoutSet;
		_clientHeaderBufferSize = other._clientHeaderBufferSize;
		_clientHeaderBufferSizeSet = other._clientHeaderBufferSizeSet;

		// Deep copy servers
		for (std::map<ServerKey, Server *>::const_iterator it = other._servers.begin();
			 it != other._servers.end(); ++it)
		{
			_servers[it->first] = new Server(*it->second);
		}
	}
	return *this;
}

void WebServer::cleanupServers()
{
	// Use a set to track unique Server pointers
	std::set<Server *> unique_servers;

	// Collect unique server pointers
	for (std::map<ServerKey, Server *>::iterator it = _servers.begin();
		 it != _servers.end(); ++it)
	{
		unique_servers.insert(it->second);
	}

	// Delete each unique server only once
	for (std::set<Server *>::iterator it = unique_servers.begin();
		 it != unique_servers.end(); ++it)
	{
		delete *it;
	}

	_servers.clear();
}

void WebServer::cleanupConnections()
{
	for (std::map<int, Connection *>::iterator it = _connections.begin();
		 it != _connections.end(); ++it)
	{
		delete it->second;
		if (close(it->first) == -1)
		{
			int err = errno;
			std::cerr << "close (" << it->first << "): " << strerror(err) << std::endl;
		}
	}
	_connections.clear();
}

WebServer::~WebServer()
{
	cleanupConnections();
	closeListenerSockets();
	if (_epfd != -1 && close(_epfd) == -1)
	{
		int err = errno;
		std::cerr << "close (" << _epfd << "): " << strerror(err) << std::endl;
	}
	_epfd = -1;
	cleanupServers();
}

void WebServer::setClientTimeout(int timeout)
{
	_clientTimeout = timeout;
	_clientTimeoutSet = true;
}

int WebServer::getClientTimeout() const
{
	return _clientTimeout;
}

bool WebServer::isClientTimeoutSet() const
{
	return _clientTimeoutSet;
}

void WebServer::setClientHeaderBufferSize(const std::string &size)
{
	_clientHeaderBufferSize = convertSizeToBytes(size);
	_clientHeaderBufferSizeSet = true;
}

int WebServer::getClientHeaderBufferSize() const
{
	return _clientHeaderBufferSize;
}

bool WebServer::isClientHeaderBufferSizeSet() const
{
	return _clientHeaderBufferSizeSet;
}

std::string WebServer::readUntilDelimiter(std::istream &file, const std::string &delimiters)
{
	std::string result;
	char c;

	while (file.get(c))
	{
		// If we hit a comment, skip to end of line
		if (c == '#')
		{
			std::string comment_line;
			std::getline(file, comment_line);
			continue;
		}

		// Check for delimiter
		if (delimiters.find(c) != std::string::npos)
		{
			result += c;
			break;
		}

		// Add char to result if not in comment
		result += c;
	}
	return result;
}

void WebServer::handleOpenBracket(const std::string &content_block, Server *&curr_server, Location *&curr_location, ParseState &state)
{
	std::vector<std::string> words = splitByWhiteSpaces(content_block);
	if (words.size() == 0)
		throw std::invalid_argument("Invalid content block");
	if (words[0] == "server")
	{
		if (words.size() != 1)
			throw std::invalid_argument("Invalid server block");
		if (state != GLOBAL)
			throw std::invalid_argument("Server not at global level");
		curr_server = new Server();
		state = SERVER;
	}
	else if (words[0] == "location")
	{
		if (words.size() != 2)
			throw std::invalid_argument("Invalid location block");
		if (state != SERVER)
			throw std::invalid_argument("Location not inside server block");
		if (!isValidAbsolutePath(words[1]))
			throw std::invalid_argument("Invalid path in location block: " + words[1]);
		curr_location = new Location(words[1]);
		curr_server->addLocation(curr_location);
		state = LOCATION;
	}
	else // unknown keyword with '{'
	{
		throw std::invalid_argument("Invalid content block");
	}
}

void WebServer::resolveAndAddListen(const std::string &listen, Server *server)
{
	if (listen.empty() || listen == ":" || listen == "[]:")
		throw std::invalid_argument("Empty listen directive");

	std::string host;
	std::string port = kDefaultPort;

	// Handle IPv6 address format [::]:port or [::]
	if (listen[0] == '[')
	{
		size_t closing_bracket = listen.find(']');
		if (closing_bracket == std::string::npos)
			throw std::invalid_argument("Invalid IPv6 address format: missing closing bracket");

		host = listen.substr(1, closing_bracket - 1);

		// Check if it's a valid IPv6 address including [::]
		struct in6_addr addr6;
		if (inet_pton(AF_INET6, host.c_str(), &addr6) != 1)
			throw std::invalid_argument("Invalid IPv6 address format");

		if (closing_bracket + 1 < listen.length())
		{
			if (listen[closing_bracket + 1] != ':')
				throw std::invalid_argument("Invalid IPv6 address format: expected ':' after closing bracket");
			if (closing_bracket + 2 >= listen.length())
				throw std::invalid_argument("Empty port in listen directive");
			port = listen.substr(closing_bracket + 2);
		}

		// If it's just [::] or [valid_ipv6], use default port
		std::string resolved_address = "[" + host + "]:" + port;
		server->addListen(resolved_address);
		return;
	}
	else
	{
		// Existing IPv4 and hostname handling
		size_t colon_pos = listen.find(':');
		if (colon_pos != std::string::npos)
		{
			host = listen.substr(0, colon_pos);
			port = listen.substr(colon_pos + 1);
		}
		else
		{
			// If no colon, check if the whole string is a port number
			bool is_port = true;
			for (size_t i = 0; i < listen.length(); ++i)
			{
				if (!isdigit(listen[i]))
				{
					is_port = false;
					break;
				}
			}

			if (is_port)
			{
				std::string full_address = kDefaultHost + ":" + listen;
				server->addListen(full_address);
				return;
			}
			host = listen; // Whole string is host with default port
		}
	}

	// Validate port
	if (!port.empty())
	{
		for (size_t i = 0; i < port.length(); ++i)
		{
			if (!isdigit(port[i]))
				throw std::invalid_argument("Invalid port number in listen directive");
		}
		int port_num = atoi(port.c_str());
		if (port_num <= 0 || port_num > 65535)
			throw std::invalid_argument("Port number out of range (1-65535)");
	}

	// Resolve and validate host if present
	if (!host.empty())
	{
		int rv;
		std::string strerr;
		struct addrinfo hints, *ai, *p;

		// Get us a socket and bind it
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM;

		if ((rv = getaddrinfo(host.c_str(), port.c_str(), &hints, &ai)) != 0)
		{
			strerr = "getaddrinfo(" + host + ":" + port + "): " + std::string(gai_strerror(rv));
			throw std::runtime_error(strerr);
		}

		if (DEBUG)
			printAddrinfo(host.c_str(), port.c_str(), ai);

		for (p = ai; p != NULL; p = p->ai_next)
		{
			std::string straddr = getStraddr(p);
			std::string resolved_address;
			if (p->ai_family == AF_INET) // IPv4
				resolved_address = straddr + ":" + port;
			if (p->ai_family == AF_INET6) // IPv6
				resolved_address = "[" + straddr + "]:" + port;
			server->addListen(resolved_address);
		}
		freeaddrinfo(ai); // All done with this structure
	}
}

void WebServer::validateSizeFormat(const std::string &size)
{
	if (size.empty())
		throw std::invalid_argument("Empty size value");

	// Check if the size is just a number (bytes)
	bool is_bytes = true;
	for (size_t i = 0; i < size.length(); ++i)
	{
		if (!isdigit(size[i]))
		{
			is_bytes = false;
			break;
		}
	}

	if (is_bytes)
	{
		int number = atoi(size.c_str());
		if (number <= 0)
			throw std::invalid_argument("Size in bytes must be positive");
		return;
	}

	// Check for k/K/m/M suffix
	size_t len = size.length();
	if (len < 2)
		throw std::invalid_argument("Size format must be a number followed by k/K/m/M or just a number for bytes");

	// Check that all characters except last are digits
	for (size_t i = 0; i < len - 1; ++i)
	{
		if (!isdigit(size[i]))
			throw std::invalid_argument("Invalid character in size: must be digits followed by k/K/m/M");
	}

	// Check the suffix
	char suffix = size[len - 1];
	if (suffix != 'k' && suffix != 'K' && suffix != 'm' && suffix != 'M')
		throw std::invalid_argument("Invalid size suffix: must be k/K for kilobytes or m/M for megabytes");

	// Convert number part to int and verify it's positive
	std::string num_part = size.substr(0, len - 1);
	int number = atoi(num_part.c_str());
	if (number <= 0)
		throw std::invalid_argument("Size value must be positive");
}

void WebServer::handle_cgi_bin_directive(const std::vector<std::string> &words, Server *curr_server)
{
	if (words.size() != 3)
		throw std::invalid_argument("Invalid cgi_bin directive: must be 'cgi_bin <extension> <path>'");

	// Validate extension format
	const std::string &extension = words[1];
	if (extension.empty() || extension[0] != '.')
		throw std::invalid_argument("Invalid cgi_bin extension: must start with '.'");

	// Validate that extension contains only alphanumeric characters
	for (size_t i = 1; i < extension.length(); ++i)
	{
		if (!isalnum(extension[i]))
			throw std::invalid_argument("Invalid cgi_bin extension: must contain only alphanumeric characters");
	}

	// Validate CGI executable path
	const std::string &cgi_path = words[2];
	if (!isValidAbsolutePath(cgi_path))
		throw std::invalid_argument("Invalid cgi_bin path: must be absolute path");

	curr_server->addCgiBin(extension, cgi_path);
}

void WebServer::validateUrl(const std::string &url) const
{
	if (url.empty())
		throw std::invalid_argument("Empty URL");

	// Check for valid protocol
	if (url.substr(0, 7) != "http://" && url.substr(0, 8) != "https://")
		throw std::invalid_argument("Invalid protocol: must be http:// or https://");

	size_t protocol_end = (url.substr(0, 5) == "https") ? 8 : 7;

	// Find end of hostname (first '/' after protocol)
	size_t path_start = url.find('/', protocol_end);
	if (path_start == std::string::npos)
		path_start = url.length();

	// Extract hostname
	std::string hostname = url.substr(protocol_end, path_start - protocol_end);

	// Hostname checks
	if (hostname.empty())
		throw std::invalid_argument("Empty hostname in URL");

	// Check for invalid characters in hostname
	for (size_t i = 0; i < hostname.length(); ++i)
	{
		char c = hostname[i];
		if (!isalnum(c) && c != '.' && c != '-' && c != '_')
			throw std::invalid_argument("Invalid character in hostname: '" + std::string(1, c) + "'");
	}

	// Check for invalid characters in path
	if (path_start < url.length())
	{
		std::string path = url.substr(path_start);
		for (size_t i = 0; i < path.length(); ++i)
		{
			char c = path[i];
			if (!isalnum(c) && c != '/' && c != '-' && c != '_' &&
				c != '.' && c != '~' && c != '%' && c != '+' && c != '=')
				throw std::invalid_argument("Invalid character in path: '" + std::string(1, c) + "'");
		}
	}
}

void WebServer::validateReturnDirective(const std::vector<std::string> &words)
{
	if (words.size() != 3)
		throw std::invalid_argument("Invalid return directive format: must be 'return <code> <URL or \"text\">'");

	// Validate status code
	if (!isNumber(words[1]))
		throw std::invalid_argument("Invalid return directive: status code must be numeric");

	int code = atoi(words[1].c_str());
	if (code < 100 || code > 599)
		throw std::invalid_argument("Invalid return directive: status code must be between 100 and 599");

	// Check for either quoted text or URL
	const std::string &target = words[2];
	if (target[0] == '"' && target[target.length() - 1] == '"')
	{
		return;
	}

	// Validate URL format
	validateUrl(target);
}

void WebServer::validateRootDirective(const std::vector<std::string> &words)
{
	if (words.size() != 2)
		throw std::invalid_argument("Invalid root directive");
	if (!isValidAbsolutePath(words[1]))
		throw std::invalid_argument("Invalid path in root directive");
}

void WebServer::handleServerDirective(const std::vector<std::string> &words, Server *curr_server)
{
	if (words[0] == "listen")
	{
		if (words.size() != 2)
			throw std::invalid_argument("Invalid listen directive");
		resolveAndAddListen(words[1], curr_server);
	}
	else if (words[0] == "server_name")
	{
		if (words.size() < 2)
			throw std::invalid_argument("Invalid server_name directive");
		for (size_t i = 1; i < words.size(); ++i)
			curr_server->addServerName(words[i]);
	}
	else if (words[0] == "root")
	{
		if (curr_server->isRootSet())
			throw std::invalid_argument("Duplicate root directive");
		validateRootDirective(words);
		curr_server->setRoot(words[1]);
	}
	else if (words[0] == "index")
	{
		if (words.size() < 2)
			throw std::invalid_argument("Invalid index directive");
		for (size_t i = 1; i < words.size(); ++i)
			curr_server->addIndex(words[i]);
	}
	else if (words[0] == "client_max_body_size")
	{
		if (words.size() != 2)
			throw std::invalid_argument("Invalid client_max_body_size directive");
		if (curr_server->isClientMaxBodySizeSet())
			throw std::invalid_argument("Duplicate client_max_body_size directive");
		validateSizeFormat(words[1]);
		curr_server->setClientMaxBodySize(words[1]);
	}
	else if (words[0] == "error_page")
	{
		if (words.size() < 3)
			throw std::invalid_argument("Invalid error_page directive");
		if (!isValidAbsolutePath(words[words.size() - 1]))
			throw std::invalid_argument("Invalid path in error_page directive");
		for (size_t i = 1; i < words.size() - 1; ++i)
		{
			if (!isNumber(words[i]))
				throw std::invalid_argument("Invalid error code in error_page directive");
			int code = atoi(words[i].c_str());
			if (code < 300 || code > 599)
				throw std::invalid_argument("Invalid error code in error_page directive");
			curr_server->addErrorPage(code, words[words.size() - 1]);
		}
	}
	else if (words[0] == "allowed_methods")
	{
		if (words.size() < 2)
			throw std::invalid_argument("Invalid allowed_methods directive: no methods specified");
		if (curr_server->isAllowedMethodsSet())
			throw std::invalid_argument("Duplicate allowed_methods directive");
		for (size_t i = 1; i < words.size(); ++i)
		{
			if (kDefaultAllowedMethods.find(words[i]) == kDefaultAllowedMethods.end())
				throw std::invalid_argument("Invalid method in allowed_methods directive: " + words[i]);
			curr_server->addAllowedMethod(words[i]);
		}
	}
	else if (words[0] == "autoindex")
	{
		if (words.size() != 2)
			throw std::invalid_argument("Invalid autoindex directive");
		if (curr_server->isAutoindexSet())
			throw std::invalid_argument("Duplicate autoindex directive");
		if (words[1] == "on")
			curr_server->setAutoindex(true);
		else if (words[1] == "off")
			curr_server->setAutoindex(false);
		else
			throw std::invalid_argument("Invalid autoindex directive value: " + words[1]);
	}
	else if (words[0] == "cgi_bin")
	{
		handle_cgi_bin_directive(words, curr_server);
	}
	else if (words[0] == "return")
	{
		validateReturnDirective(words);
		curr_server->setReturnDirective(words[1]);
	}
	else
	{
		throw std::invalid_argument("Invalid directive in server block: " + words[0]);
	}
}

void WebServer::handleLocationDirective(const std::vector<std::string> &words, Location *curr_location)
{
	if (words[0] == "root")
	{
		if (curr_location->isRootSet())
			throw std::invalid_argument("Duplicate root directive");
		validateRootDirective(words);
		curr_location->setRoot(words[1]);
	}
	else if (words[0] == "index")
	{
		if (words.size() < 2)
			throw std::invalid_argument("Invalid index directive");
		for (size_t i = 1; i < words.size(); ++i)
			curr_location->addIndex(words[i]);
	}
	else if (words[0] == "autoindex")
	{
		if (words.size() != 2)
			throw std::invalid_argument("Invalid autoindex directive");
		if (curr_location->isAutoindexSet())
			throw std::invalid_argument("Duplicate autoindex directive");
		if (words[1] == "on")
			curr_location->setAutoindex(true);
		else if (words[1] == "off")
			curr_location->setAutoindex(false);
		else
			throw std::invalid_argument("Invalid autoindex directive value: " + words[1]);
	}
	else if (words[0] == "return")
	{
		validateReturnDirective(words);
		curr_location->setReturnDirective(words[1]);
	}
	else if (words[0] == "upload_directory")
	{
		if (words.size() != 2)
			throw std::invalid_argument("Invalid upload_directory directive");
		if (curr_location->isUploadDirectorySet())
			throw std::invalid_argument("Duplicate upload_directory directive");
		if (!isValidAbsolutePath(words[1]))
			throw std::invalid_argument("Invalid path in upload_directory directive");
		curr_location->setUploadDirectory(words[1]);
	}
	else if (words[0] == "allowed_methods")
	{
		if (words.size() < 2)
			throw std::invalid_argument("Invalid allowed_methods directive: no methods specified");
		if (curr_location->isAllowedMethodSet())
			throw std::invalid_argument("Duplicate allowed_methods directive");
		for (size_t i = 1; i < words.size(); ++i)
		{
			if (kDefaultAllowedMethods.find(words[i]) == kDefaultAllowedMethods.end())
				throw std::invalid_argument("Invalid method in allowed_methods directive: " + words[i]);
			curr_location->addAllowedMethod(words[i]);
		}
	}
	else
	{
		throw std::invalid_argument("Invalid directive in location block: " + words[0]);
	}
}

void WebServer::handleGlobalDirective(const std::vector<std::string> &words)
{
	if (words[0] == "client_timeout")
	{
		if (words.size() != 2)
			throw std::invalid_argument("Invalid client_timeout directive");
		if (isClientTimeoutSet())
			throw std::invalid_argument("Duplicate client_timeout directive");
		if (!isNumber(words[1]))
			throw std::invalid_argument("client_timeout is not numeric");
		int timeout = atoi(words[1].c_str());
		if (timeout <= 0)
			throw std::invalid_argument("Invalid timeout in client_timeout directive");
		setClientTimeout(timeout);
	}
	else if (words[0] == "client_header_buffer_size")
	{
		if (words.size() != 2)
			throw std::invalid_argument("Invalid client_header_buffer_size directive");
		if (isClientHeaderBufferSizeSet())
			throw std::invalid_argument("Duplicate client_header_buffer_size directive");
		validateSizeFormat(words[1]);
		this->setClientHeaderBufferSize(words[1]);
	}
	else
	{
		throw std::invalid_argument("Invalid directive in global block: " + words[0]);
	}
}

void WebServer::handleDirective(const std::string &content_block, Server *curr_server, Location *curr_location, ParseState &state)
{
	std::vector<std::string> words = splitByWhiteSpaces(content_block);
	if (words.size() == 0)
		throw std::invalid_argument("unexpected \";\"");
	switch (state)
	{
	case GLOBAL:
		handleGlobalDirective(words);
		break;
	case SERVER:
		handleServerDirective(words, curr_server);
		break;
	case LOCATION:
		handleLocationDirective(words, curr_location);
		break;
	default:
		throw std::invalid_argument("Invalid state");
	}
}

void WebServer::inheritServerDirectives(Server *curr_server)
{
	std::vector<Location *> locations = curr_server->getLocations();
	for (size_t i = 0; i < locations.size(); ++i)
	{
		Location *loc = locations[i];

		// Inherit root if not set in location
		if (!loc->isRootSet() && curr_server->isRootSet())
			loc->setRoot(curr_server->getRoot());

		// Inherit index if not set in location
		if (!loc->isIndexSet() && curr_server->isIndexSet())
		{
			const std::set<std::string> &serverIndex = curr_server->getIndex();
			for (std::set<std::string>::const_iterator idx = serverIndex.begin();
				 idx != serverIndex.end(); ++idx)
			{
				loc->addIndex(*idx);
			}
		}

		// Inherit allowed methods if not set in location
		if (!loc->isAllowedMethodSet() && curr_server->isAllowedMethodsSet())
		{
			const std::map<std::string, bool> &serverMethods = curr_server->getAllowedMethods();
			for (std::map<std::string, bool>::const_iterator method = serverMethods.begin();
				 method != serverMethods.end(); ++method)
			{
				if (method->second) // Only add if true
					loc->addAllowedMethod(method->first);
			}
		}

		// Inherit autoindex if not set in location
		if (!loc->isAutoindexSet() && curr_server->isAutoindexSet())
			loc->setAutoindex(curr_server->getAutoindex());
	}
}

void WebServer::addServer(Server *server)
{
	if (!server)
		return;

	const std::set<std::string> &listens = server->getListens();
	const std::set<std::string> &names = server->getServerNames();

	// For each listen directive
	for (std::set<std::string>::const_iterator it = listens.begin();
		 it != listens.end(); ++it)
	{
		// Parse address and port from listen directive
		std::string addr = *it;
		size_t colon_pos = addr.rfind(':');
		if (colon_pos == std::string::npos)
			continue; // Invalid format, skip

		std::string host = addr.substr(0, colon_pos);
		std::string port = addr.substr(colon_pos + 1);

		// Remove brackets from IPv6 address if present
		if (host.length() > 2 && host[0] == '[' && host[host.length() - 1] == ']')
			host = host.substr(1, host.length() - 2);

		// Add default server for this host:port if not already present
		ServerKey default_key(port, host, kDefaultServerName);
		if (_servers.find(default_key) == _servers.end())
			_servers[default_key] = server;

		// If server names specified, add named servers if not already present
		if (!names.empty())
		{
			for (std::set<std::string>::const_iterator name = names.begin();
				 name != names.end(); ++name)
			{
				ServerKey named_key(port, host, *name);
				if (_servers.find(named_key) == _servers.end())
					_servers[named_key] = server;
			}
		}
	}
}

void WebServer::handleCloseBracket(const std::string &content_block, Server *&curr_server, Location *&curr_location, ParseState &state)
{
	std::vector<std::string> words = splitByWhiteSpaces(content_block);
	if (words.size() != 0)
		throw std::invalid_argument("Invalid content after closing bracket: " + content_block);
	switch (state)
	{
	case GLOBAL:
		throw std::invalid_argument("Closing bracket without opening bracket");
	case SERVER:
		inheritServerDirectives(curr_server);
		addServer(curr_server);
		curr_server = NULL;
		state = GLOBAL;
		break;
	case LOCATION:
		(void)curr_location; // Unused warning
		curr_location = NULL;
		state = SERVER;
		break;
	default:
		throw std::invalid_argument("Invalid state");
	}
}

void WebServer::parseConfig()
{
	std::ifstream file(this->_fileName.c_str());
	if (!file.is_open())
		throw std::invalid_argument("File not found: " + this->_fileName);

	ParseState state = GLOBAL;
	Server *curr_server = NULL;
	Location *curr_location = NULL;
	const std::string delimiters = ";{}";

	try
	{
		std::string content_block = this->readUntilDelimiter(file, delimiters);
		content_block = trim(content_block);
		while (!content_block.empty())
		{
			char delimiter = content_block[content_block.size() - 1];
			content_block.erase(content_block.size() - 1);
			switch (delimiter)
			{
			case '{':
				handleOpenBracket(content_block, curr_server, curr_location, state);
				break;
			case ';':
				handleDirective(content_block, curr_server, curr_location, state);
				break;
			case '}':
				handleCloseBracket(content_block, curr_server, curr_location, state);
				break;
			default: // no delimiter
				throw std::invalid_argument("Invalid block");
			}
			content_block = this->readUntilDelimiter(file, delimiters);
			content_block = trim(content_block);
		}
		if (state != GLOBAL)
			throw std::invalid_argument("Missing closing bracket");
		file.close();
	}
	catch (const std::exception &e)
	{
		// Clean up current server if it exists and hasn't been added to _servers yet
		if (curr_server)
		{
			bool server_exists = false;
			// Check if server exists in _servers map
			for (std::map<ServerKey, Server *>::iterator it = _servers.begin();
				 it != _servers.end() && !server_exists; ++it)
			{
				if (it->second == curr_server)
				{
					server_exists = true;
				}
			}

			// Delete only if server wasn't added to _servers
			if (!server_exists)
			{
				delete curr_server;
			}
		}
		// current location cleanup is not needed because it's added to the server
		// and server cleanup is handled above
		cleanupServers();
		file.close();
		throw;
	}
}

void WebServer::setNonblocking(int fd)
{
	// TODO: didn't use F_GETFL because by default it appends new flags to the existing ones
	// so not sure if I should use it or not.
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		int err = errno;
		// TODO: print error? or should throw exception? or just close the socket?
		std::cerr << "fcntl O_NONBLOCK error (" << fd << "): " << strerror(err) << std::endl;
	}
}

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) // IPv4
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr); // IPv6
}

void WebServer::closeListenerSockets()
{
	// Close all listener sockets
	for (std::map<int, std::pair<std::string, std::string> >::iterator it = _listeners.begin();
		 it != _listeners.end(); ++it)
	{
		if (close(it->first) == -1)
		{
			int err = errno;
			std::cerr << "close (listener " << it->first << "): "
					  << strerror(err) << std::endl;
		}
	}
	_listeners.clear();
}

void WebServer::setupListenerSockets()
{
	int yes = 1; // For setsockopt() SO_REUSEADDR, below
	int rv;
	std::string strerr;
	struct addrinfo hints, *ai;
	// set of bound addresses for IPv4 and IPv6 where pair.first is the address and pair.second is the port
	std::set<std::pair<std::string, std::string> > bound_addresses;

	// Get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	for (std::map<ServerKey, Server *>::const_iterator it = _servers.begin();
		 it != _servers.end(); ++it)
	{
		const ServerKey &key = it->first;
		if ((rv = getaddrinfo(key.host.c_str(), key.port.c_str(), &hints, &ai)) != 0)
		{
			strerr = "getaddrinfo(" + key.host + ":" + key.port + "): " + std::string(gai_strerror(rv));
			throw std::runtime_error(strerr);
		}

		if (ai->ai_family == AF_INET) // IPv4
		{
			if (bound_addresses.find(std::make_pair("0.0.0.0", key.port)) != bound_addresses.end())
			{
				freeaddrinfo(ai);
				continue;
			}
		}
		if (ai->ai_family == AF_INET6) // IPv6
		{
			if (bound_addresses.find(std::make_pair("::", key.port)) != bound_addresses.end())
			{
				freeaddrinfo(ai);
				continue;
			}
		}
		if (bound_addresses.find(std::make_pair(key.host, key.port)) != bound_addresses.end())
		{
			freeaddrinfo(ai);
			continue;
		}

		int listener = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (listener < 0)
		{
			int err = errno;
			strerr = "socket(" + key.host + ":" + key.port + "): " + strerror(err);
			closeListenerSockets();
			freeaddrinfo(ai);
			throw std::runtime_error(strerr);
		}
		// Avoid error of "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); // don't care about return value, do the best effort
		if (bind(listener, ai->ai_addr, ai->ai_addrlen) < 0)
		{
			int err = errno;
			strerr = "bind(" + key.host + ":" + key.port + "): " + strerror(err);
			if (close(listener) == -1)
			{
				err = errno;
				std::cerr << "close (" << listener << "): " << strerror(err) << std::endl;
			}
			closeListenerSockets();
			freeaddrinfo(ai);
			throw std::runtime_error(strerr);
		}

		// Set the socket to be non-blocking
		setNonblocking(listener);

		// Listen
		if (listen(listener, 10) == -1)
		{
			int err = errno;
			strerr = "listen(" + key.host + ":" + key.port + "): " + strerror(err);
			if (close(listener) == -1)
			{
				err = errno;
				std::cerr << "close (" << listener << "): " << strerror(err) << std::endl;
			}
			closeListenerSockets();
			freeaddrinfo(ai);
			throw std::runtime_error(strerr);
		}
		_listeners[listener] = std::make_pair(key.host, key.port);
		bound_addresses.insert(std::make_pair(key.host, key.port));
		freeaddrinfo(ai); // All done with this structure
	}
	if (DEBUG)
	{
		std::cout << "Listening on the following addresses:" << std::endl;
		for (std::map<int, std::pair<std::string, std::string> >::iterator it = _listeners.begin();
			 it != _listeners.end(); ++it)
		{
			std::cout << it->second.first << ":" << it->second.second << std::endl;
		}
	}
}

void WebServer::handleNewConnection(int listener)
{
	socklen_t addrlen;
	struct sockaddr_storage remoteaddr; // Client address
	int newfd;							// Newly accept()ed socket descriptor
	char remoteIP[INET6_ADDRSTRLEN];

	// If listener is ready to read, handle new connection
	addrlen = sizeof remoteaddr;
	newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
	if (newfd == -1)
	{
		perror("accept");
		return;
	}
	// Set the new socket to non-blocking mode
	setNonblocking(newfd);
	struct epoll_event ev;
	//TODO: handle EPOLLOUT too and maybe others
	ev.events = EPOLLIN;
	ev.data.fd = newfd;
	if (epoll_ctl(_epfd, EPOLL_CTL_ADD, newfd, &ev) == -1)
	{
		perror("epoll_ctl: newfd");
		if (close(newfd) == -1)
		{
			perror("close newfd");
		}
	}
	// Print info about the new connection
	std::stringstream ss;
	ss << "new connection " << inet_ntop(remoteaddr.ss_family,
										   get_in_addr((struct sockaddr *)&remoteaddr),
										   remoteIP, INET6_ADDRSTRLEN)
	   << ":" << ntohs(((struct sockaddr_in *)&remoteaddr)->sin_port)
	   << " -> " << _listeners[listener].first << ":" << _listeners[listener].second
	   << " socket " << newfd;
	std::cout << ss.str() << std::endl;

	// Add the new connection to the map of connections
	_connections[newfd] = new Connection(newfd, _listeners[listener].first, _listeners[listener].second);
}

void WebServer::handleClientData(int sender_fd)
{
	char buf[kMaxBuff]; // Buffer for client data

	int nbytes = recv(sender_fd, buf, sizeof buf, 0);
	if (nbytes <= 0)
	{
		// Got error or connection closed by client
		if (nbytes == 0)
		{
			// Connection closed
			std::cout << "socket " << sender_fd << " hung up" << std::endl;
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			// No data available
			return;
		}
		else
		{
			perror("recv");
		}
		handleConnectionClose(sender_fd);
	}
	else
	{
		// TODO: implement EPOLLOUT logic with EAGAIN and EWOULDBLOCK
		// Send a dummy HTTP response
		const char *http_response =
			"HTTP/1.1 200 OK\r\n"
			"Date: Fri, 07 Feb 2025 21:00:00 GMT\r\n"
			"Server: MyWebServer/1.0\r\n"
			"Content-Length: 13\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n"
			"\r\n"
			"Hello, world!";

		int response_len = strlen(http_response); // not sure we can use it
		if (send(sender_fd, http_response, response_len, 0) == -1)
		{
			perror("send");
		}
	}
}

void WebServer::handleConnectionClose(int fd)
{
	if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
	{
		perror("epoll_ctl: del error fd");
	}
	// Check if the fd is in the connections map or in the listeners map
	std::map<int, Connection*>::iterator it = _connections.find(fd);
	if (it != _connections.end())
	{
		delete it->second;
		_connections.erase(it);
	}
	std::map<int, std::pair<std::string, std::string> >::iterator it2 = _listeners.find(fd);
	if (it2 != _listeners.end())
	{
		_listeners.erase(it2);
	}
	// Close the socket
	if (close(fd) == -1)
	{
		perror("close fd");
	}
}

void WebServer::processPollEvents(int ready)
{
	for (int i = 0; i < ready; i++)
	{
		if (_evlist[i].events & EPOLLIN)
		{
			if (_listeners.find(_evlist[i].data.fd) != _listeners.end()) // the fd is a listener and ready to read
			{
				// If listener is ready to read, handle new connection
				handleNewConnection(_evlist[i].data.fd);
			}
			else
			{
				// If not the listener, we're just a regular client
				handleClientData(_evlist[i].data.fd);
			}
		}
		else if (_evlist[i].events & EPOLLHUP)
		{
			// TODO: handle listen socket errors and client socket close/errors
			//  An error has occured on this fd, or the socket was closed
			std::cout << "epoll: hangup on fd " << _evlist[i].data.fd << std::endl;
			handleConnectionClose(_evlist[i].data.fd);
		}
		else if (_evlist[i].events & EPOLLERR)
		{
			// An error has occured on this fd
			std::cerr << "epoll: error on fd " << _evlist[i].data.fd << std::endl;
			handleConnectionClose(_evlist[i].data.fd);
		}
		// TODO: handle EPOLLOUT too
	}
}

void WebServer::initEpoll()
{
	this->_epfd = epoll_create(42);
	if (this->_epfd == -1)
	{
		perror("epoll_create");
		throw std::runtime_error("epoll_create");
	}

	for (std::map<int, std::pair<std::string, std::string> >::iterator it = _listeners.begin();
		 it != _listeners.end(); ++it)
	{
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = it->first;
		if (epoll_ctl(this->_epfd, EPOLL_CTL_ADD, it->first, &ev) == -1)
		{
			int err = errno;
			perror("epoll_ctl: listener_sock");
			if (close(this->_epfd) == -1)
			{
				perror("close _epfd");
			}
			_epfd = -1;
			std::stringstream ss;
			ss << "epoll_ctl: failed to add listener socket " << it->first << ": " << strerror(err);
			throw std::runtime_error(ss.str());
		}
	}
}

void WebServer::printSettings() const
{
	std::cout << "\n=== WebServer Configuration ===\n";

	if (_servers.empty())
	{
		std::cout << "[EMPTY - No servers configured]\n";
		return;
	}
	std::cout << "Servers Keys:\n";
	for (std::map<ServerKey, Server *>::const_iterator it = _servers.begin();
		 it != _servers.end(); ++it)
	{
		const ServerKey &key = it->first;
		if (key.server_name == "")
			std::cout << "  Port: " << key.port << ", Host: " << key.host << ", Name: [DEFAULT SERVER] -> " << it->second << "\n";
		else
			std::cout << "  Port: " << key.port << ", Host: " << key.host << ", Name: " << key.server_name << " -> " << it->second << "\n";
	}

	std::set<Server *> printed_servers;

	for (std::map<ServerKey, Server *>::const_iterator it = _servers.begin();
		 it != _servers.end(); ++it)
	{
		Server *server = it->second;
		if (printed_servers.find(server) != printed_servers.end())
			continue;

		printed_servers.insert(server);
		std::cout << "\nServer Block:\n";

		// Listen directives
		std::cout << "  Listen directives:\n";
		const std::set<std::string> &listens = server->getListens();
		if (listens.empty())
			std::cout << "    [EMPTY]\n";
		else
			for (std::set<std::string>::const_iterator l = listens.begin();
				 l != listens.end(); ++l)
				std::cout << "    " << *l << "\n";

		// Server names
		std::cout << "  Server names:\n";
		const std::set<std::string> &names = server->getServerNames();
		if (names.empty())
			std::cout << "    [EMPTY - Using default server]\n";
		else
			for (std::set<std::string>::const_iterator n = names.begin();
				 n != names.end(); ++n)
			{
				if (*n == "")
					std::cout << "    [DEFAULT SERVER]\n";
				else
					std::cout << "    " << *n << "\n";
			}

		// Root
		if (server->isRootSet())
			std::cout << "  Root: " << server->getRoot() << "\n";
		else
			std::cout << "  Root: [NOT SET]\n";

		// Index files
		std::cout << "  Index files:\n";
		if (!server->isIndexSet())
			std::cout << "    [NOT SET]\n";
		else
		{
			const std::set<std::string> &indices = server->getIndex();
			if (indices.empty())
				std::cout << "    [EMPTY]\n";
			else
				for (std::set<std::string>::const_iterator i = indices.begin();
					 i != indices.end(); ++i)
					std::cout << "    " << *i << "\n";
		}

		// Locations
		std::cout << "  Locations:\n";
		std::vector<Location *> locations = server->getLocations();
		if (locations.empty())
		{
			std::cout << "    [EMPTY - No locations configured]\n";
			continue;
		}

		for (size_t i = 0; i < locations.size(); ++i)
		{
			Location *loc = locations[i];
			std::cout << "    Location " << loc->getPath() << ":\n";

			if (loc->isRootSet())
				std::cout << "      Root: " << loc->getRoot() << "\n";
			else
				std::cout << "      Root: [INHERITED]\n";

			std::cout << "      Index files:\n";
			if (!loc->isIndexSet())
				std::cout << "        [INHERITED]\n";
			else
			{
				const std::set<std::string> &loc_indices = loc->getIndex();
				if (loc_indices.empty())
					std::cout << "        [EMPTY]\n";
				else
					for (std::set<std::string>::const_iterator li = loc_indices.begin();
						 li != loc_indices.end(); ++li)
						std::cout << "        " << *li << "\n";
			}

			if (loc->isAutoindexSet())
				std::cout << "      Autoindex: " << (loc->getAutoindex() ? "on" : "off") << "\n";
			else
				std::cout << "      Autoindex: [INHERITED]\n";

			std::cout << "      Allowed methods:\n";
			if (!loc->isAllowedMethodSet())
				std::cout << "        [INHERITED]\n";
			else
			{
				const std::map<std::string, bool> &methods = loc->getAllowedMethods();
				if (methods.empty())
					std::cout << "        [EMPTY]\n";
				else
					for (std::map<std::string, bool>::const_iterator m = methods.begin();
						 m != methods.end(); ++m)
						if (m->second)
							std::cout << "        " << m->first << "\n";
			}
		}
	}
	std::cout << "\n===========================\n";
}

void WebServer::closeExpiredConnections()
{
	time_t current_time = time(NULL);
	if (current_time == static_cast<time_t>(-1))
	{
		perror("time");
		return;
	}

	std::vector<int> timed_out_fds;

	// If we delete connections from the map while iterating over it, it will invalidate the iterator
	// so we first collect all timed out connections and then close them.
	for (std::map<int, Connection*>::iterator it = _connections.begin();
			it != _connections.end(); ++it)
	{
		if (current_time - it->second->getLastActivityTime() > _clientTimeout)
		{
			timed_out_fds.push_back(it->first);
		}
	}

	// Then close them
	for (std::vector<int>::iterator it = timed_out_fds.begin();
			it != timed_out_fds.end(); ++it)
	{
		std::cout << "Connection timeout on fd " << *it << std::endl;
		handleConnectionClose(*it);
	}
}

void WebServer::run()
{
	this->setupListenerSockets();
	this->initEpoll();

	// Main loop
	while (g_running) // initialized to true at header file, until a signal is received
	{
		int ready = epoll_wait(this->_epfd, _evlist, kMaxEvents, -1);
		if (ready == -1)
		{
			perror("epoll_wait");
			continue;
			// TODO: how to handle EINTR? in case of SIGCHLD after fork
			// when child process is terminated and parent registered signal handler on SIGCHLD
		}
		closeExpiredConnections();
		processPollEvents(ready);
	}
}
