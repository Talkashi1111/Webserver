#include "Location.hpp"
#include "Consts.hpp"

Location::Location() : _path(""),
					   _allowedMethods(kDefaultAllowedMethods),
					   _root(kDefaultRoot),
					   _index(kDefaultIndex.begin(), kDefaultIndex.end()),
					   _autoindex(kDefaultAutoindex),
					   _returnDirective(),
					   _uploadDirectory(""),
					   // Initialize all flags to false
					   _allowedMethodsSet(false),
					   _rootSet(false),
					   _indexSet(false),
					   _autoindexSet(false),
					   _returnDirectiveSet(false),
					   _uploadDirectorySet(false)
{
}

Location::Location(const std::string &path) : _path(path),
											  _allowedMethods(kDefaultAllowedMethods),
											  _root(kDefaultRoot),
											  _index(kDefaultIndex.begin(), kDefaultIndex.end()),
											  _autoindex(kDefaultAutoindex),
											  _returnDirective(),
											  _uploadDirectory(""),
											  // Initialize all flags to false
											  _allowedMethodsSet(false),
											  _rootSet(false),
											  _indexSet(false),
											  _autoindexSet(false),
											  _returnDirectiveSet(false),
											  _uploadDirectorySet(false)
{
}

Location::Location(const Location &other) : _path(other._path),
											_allowedMethods(other._allowedMethods),
											_root(other._root),
											_index(other._index),
											_autoindex(other._autoindex),
											_returnDirective(other._returnDirective),
											_uploadDirectory(other._uploadDirectory),
											// Copy all "isSet" flags
											_allowedMethodsSet(other._allowedMethodsSet),
											_rootSet(other._rootSet),
											_indexSet(other._indexSet),
											_autoindexSet(other._autoindexSet),
											_returnDirectiveSet(other._returnDirectiveSet),
											_uploadDirectorySet(other._uploadDirectorySet)
{
}

Location &Location::operator=(const Location &other)
{
	if (this != &other)
	{
		_path = other._path;
		_allowedMethods = other._allowedMethods;
		_root = other._root;
		_index = other._index;
		_autoindex = other._autoindex;
		_returnDirective = other._returnDirective;
		_uploadDirectory = other._uploadDirectory;
		// Copy all "isSet" flags
		_allowedMethodsSet = other._allowedMethodsSet;
		_rootSet = other._rootSet;
		_indexSet = other._indexSet;
		_autoindexSet = other._autoindexSet;
		_returnDirectiveSet = other._returnDirectiveSet;
		_uploadDirectorySet = other._uploadDirectorySet;
	}
	return *this;
}

Location::~Location()
{
	// Nothing to free explicitly.
}

void Location::setPath(const std::string &path)
{
	_path = path;
}

const std::string &Location::getPath() const
{
	return _path;
}

void Location::addAllowedMethod(const std::string &method)
{
	if (!_allowedMethodsSet)
	{
		_allowedMethods.clear();
	}
	_allowedMethods[method] = true;
	_allowedMethodsSet = true;
}

const std::map<std::string, bool> &Location::getAllowedMethods() const
{
	return _allowedMethods;
}

bool Location::isAllowedMethodSet() const
{
	return _allowedMethodsSet;
}

void Location::setRoot(const std::string &root)
{
	_root = root;
	_rootSet = true;
}

const std::string &Location::getRoot() const
{
	return _root;
}

bool Location::isRootSet() const
{
	return _rootSet;
}

void Location::addIndex(const std::string &indexFile)
{
	if (!_indexSet)
	{
		_index.clear();
	}
	_index.insert(indexFile);
	_indexSet = true;
}

const std::set<std::string> &Location::getIndex() const
{
	return _index;
}

bool Location::isIndexSet() const
{
	return _indexSet;
}

void Location::setAutoindex(bool autoindex)
{
	_autoindex = autoindex;
	_autoindexSet = true;
}

bool Location::getAutoindex() const
{
	return _autoindex;
}

bool Location::isAutoindexSet() const
{
	return _autoindexSet;
}

void Location::setReturnDirective(const std::string &statusCode, const std::string &ret)
{
	if (!_returnDirectiveSet)
	{
		_returnDirective.first = statusCode;
		_returnDirective.second = ret;
		_returnDirectiveSet = true;
	}
}

const std::pair<std::string, std::string> &Location::getReturnDirective() const
{
	return _returnDirective;
}

bool Location::isReturnDirectiveSet() const
{
	return _returnDirectiveSet;
}

void Location::setUploadDirectory(const std::string &uploadDir)
{
	_uploadDirectory = uploadDir;
	_uploadDirectorySet = true;
}

const std::string &Location::getUploadDirectory() const
{
	return _uploadDirectory;
}

bool Location::isUploadDirectorySet() const
{
	return _uploadDirectorySet;
}
