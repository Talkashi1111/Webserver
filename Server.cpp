#include <cstdlib>
#include "Server.hpp"
#include "Consts.hpp"
#include "StringUtils.hpp"

Server::Server() : _listens(),
				   _serverNames(),
				   _root(kDefaultRoot),
				   _index(kDefaultIndex.begin(), kDefaultIndex.end()),
				   _errorPages(kDefaultErrorPages),
				   _allowedMethods(kDefaultAllowedMethods),
				   _autoindex(kDefaultAutoindex),
				   _cgiBin(),
				   _returnDirective(),
				   _locationTrie(),
				   // Initialize all "isSet" flags to false
				   _listensSet(false),
				   _serverNamesSet(false),
				   _rootSet(false),
				   _indexSet(false),
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

bool Server::isListenSet(const std::string &listen) const
{
	if (!_listensSet)
		return false;
	return _listens.find(listen) != _listens.end();
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
	_rootSet = true;
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

void Server::setReturnDirective(const std::string &statusCode, const std::string &ret)
{
	if (!_returnDirectiveSet)
	{
		_returnDirective.first = statusCode;
		_returnDirective.second = ret;
		_returnDirectiveSet = true;
	}
}

const std::pair<std::string, std::string> &Server::getReturnDirective() const
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
