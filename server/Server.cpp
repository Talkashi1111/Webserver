#include <cstdlib>
#include "Server.hpp"
#include "Consts.hpp"

Server::Server() : _listens(),
				   _serverNames(),
				   _root(kDefaultRoot),
				   _index(kDefaultIndex.begin(), kDefaultIndex.end()),
				   _clientHeaderBufferSize(kDefaultClientHeaderBufferSize),
				   _clientMaxBodySize(kDefaultClientMaxBodySize),
				   _errorPages(kDefaultErrorPages),
				   _allowedMethods(kDefaultAllowedMethods),
				   _autoindex(kDefaultAutoindex),
				   _cgiBin(),
				   _returnDirective(""),
				   _locationTrie(),
				   // Initialize all "isSet" flags to false
				   _listensSet(false),
				   _serverNamesSet(false),
				   _rootSet(false),
				   _indexSet(false),
				   _clientHeaderBufferSizeSet(false),
				   _clientMaxBodySizeSet(false),
				   _errorPagesSet(),
				   _allowedMethodsSet(false),
				   _autoindexSet(false),
				   _returnDirectiveSet(false)
{
	_listens.insert(kDefaultListen);
	_serverNames.insert(kDefaultServerName);
}

Server::Server(const Server &other) : _listens(other._listens),
									  _serverNames(other._serverNames),
									  _root(other._root),
									  _index(other._index),
									  _clientHeaderBufferSize(other._clientHeaderBufferSize),
									  _clientMaxBodySize(other._clientMaxBodySize),
									  _errorPages(other._errorPages),
									  _allowedMethods(other._allowedMethods),
									  _autoindex(other._autoindex),
									  _cgiBin(other._cgiBin),
									  _returnDirective(other._returnDirective),
									  _locationTrie(other._locationTrie),
									  // Copy all "isSet" flags
									  _listensSet(other._listensSet),
									  _serverNamesSet(other._serverNamesSet),
									  _rootSet(other._rootSet),
									  _indexSet(other._indexSet),
									  _clientHeaderBufferSizeSet(other._clientHeaderBufferSizeSet),
									  _clientMaxBodySizeSet(other._clientMaxBodySizeSet),
									  _errorPagesSet(other._errorPagesSet),
									  _allowedMethodsSet(other._allowedMethodsSet),
									  _autoindexSet(other._autoindexSet),
									  _returnDirectiveSet(other._returnDirectiveSet)
{
}

Server &Server::operator=(const Server &other)
{
	if (this != &other)
	{
		_listens = other._listens;
		_serverNames = other._serverNames;
		_root = other._root;
		_index = other._index;
		_clientHeaderBufferSize = other._clientHeaderBufferSize;
		_clientMaxBodySize = other._clientMaxBodySize;
		_errorPages = other._errorPages;
		_allowedMethods = other._allowedMethods;
		_autoindex = other._autoindex;
		_cgiBin = other._cgiBin;
		_returnDirective = other._returnDirective;
		_locationTrie = other._locationTrie;
		// Copy all "isSet" flags
		_listensSet = other._listensSet;
		_serverNamesSet = other._serverNamesSet;
		_rootSet = other._rootSet;
		_indexSet = other._indexSet;
		_clientHeaderBufferSizeSet = other._clientHeaderBufferSizeSet;
		_clientMaxBodySizeSet = other._clientMaxBodySizeSet;
		_errorPagesSet = other._errorPagesSet;
		_allowedMethodsSet = other._allowedMethodsSet;
		_autoindexSet = other._autoindexSet;
		_returnDirectiveSet = other._returnDirectiveSet;
	}
	return *this;
}

Server::~Server()
{
	std::vector<Location *> locations = _locationTrie.getAllLocations();
	for (std::vector<Location *>::iterator it = locations.begin();
		 it != locations.end(); ++it)
	{
		delete *it;
	}
}

void Server::addListen(const std::string &listen)
{
	if (!_listensSet)
	{
		_listens.clear();
	}
	_listens.insert(listen);
	_listensSet = true;
}

const std::set<std::string> &Server::getListens() const
{
	return _listens;
}

bool Server::isListensSet() const
{
	return _listensSet;
}

void Server::addServerName(const std::string &name)
{
	_serverNames.insert(name);
	_serverNamesSet = true;
}

const std::set<std::string> &Server::getServerNames() const
{
	return _serverNames;
}

bool Server::isServerNamesSet() const
{
	return _serverNamesSet;
}

void Server::setRoot(const std::string &root)
{
	_root = root;
}

const std::string &Server::getRoot() const
{
	return _root;
}

bool Server::isRootSet() const
{
	return _rootSet;
}

void Server::addIndex(const std::string &indexFile)
{
	if (!_indexSet)
	{
		_index.clear();
	}
	_index.insert(indexFile);
	_indexSet = true;
}

const std::set<std::string> &Server::getIndex() const
{
	return _index;
}

bool Server::isIndexSet() const
{
	return _indexSet;
}

int Server::convertSizeToBytes(const std::string &size) const
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

void Server::setClientHeaderBufferSize(const std::string &size)
{
	_clientHeaderBufferSize = convertSizeToBytes(size);
	_clientHeaderBufferSizeSet = true;
}

int Server::getClientHeaderBufferSize() const
{
	return _clientHeaderBufferSize;
}

bool Server::isClientHeaderBufferSizeSet() const
{
	return _clientHeaderBufferSizeSet;
}

void Server::setClientMaxBodySize(const std::string &size)
{
	_clientMaxBodySize = convertSizeToBytes(size);
	_clientMaxBodySizeSet = true;
}

int Server::getClientMaxBodySize() const
{
	return _clientMaxBodySize;
}

bool Server::isClientMaxBodySizeSet() const
{
	return _clientMaxBodySizeSet;
}

void Server::addErrorPage(int code, const std::string &path)
{
	if (_errorPagesSet.find(code) == _errorPagesSet.end())
	{
		_errorPages[code] = path;
		_errorPagesSet.insert(code);
	}
}

const std::map<int, std::string> &Server::getErrorPages() const
{
	return _errorPages;
}

void Server::addAllowedMethod(const std::string &method)
{
	if (!_allowedMethodsSet)
	{
		_allowedMethods.clear();
	}
	_allowedMethods[method] = true;
	_allowedMethodsSet = true;
}

const std::map<std::string, bool> &Server::getAllowedMethods() const
{
	return _allowedMethods;
}

bool Server::isAllowedMethodsSet() const
{
	return _allowedMethodsSet;
}

void Server::setAutoindex(bool autoindex)
{
	_autoindex = autoindex;
	_autoindexSet = true;
}

bool Server::getAutoindex() const
{
	return _autoindex;
}

bool Server::isAutoindexSet() const
{
	return _autoindexSet;
}

void Server::addCgiBin(const std::string &ext, const std::string &cgiBin)
{
	_cgiBin[ext] = cgiBin;
}

const std::map<std::string, std::string> &Server::getCgiBin() const
{
	return _cgiBin;
}

void Server::setReturnDirective(const std::string &ret)
{
	_returnDirective = ret;
	_returnDirectiveSet = true;
}

const std::string &Server::getReturnDirective() const
{
	return _returnDirective;
}

bool Server::isReturnDirectiveSet() const
{
	return _returnDirectiveSet;
}

void Server::addLocation(Location *loc)
{
	if (loc)
	{
		_locationTrie.insert(loc);
	}
}

Location *Server::getLocationForURI(const std::string &uri) const
{
	return _locationTrie.searchLongestPrefix(uri);
}

std::vector<Location *> Server::getLocations() const
{
	return _locationTrie.getAllLocations();
}
