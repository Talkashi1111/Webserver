#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include "LocationTrie.hpp"

class Server
{
public:
	Server();
	Server(const Server &other);
	Server &operator=(const Server &other);
	~Server();

	// Setters / Getters
	void addListen(const std::string &listen);
	const std::set<std::string> &getListens() const;
	bool isListensSet() const;

	void addServerName(const std::string &name);
	const std::set<std::string> &getServerNames() const;
	bool isServerNamesSet() const;

	void setRoot(const std::string &root);
	const std::string &getRoot() const;
	bool isRootSet() const;

	void addIndex(const std::string &indexFile);
	const std::set<std::string> &getIndex() const;
	bool isIndexSet() const;

	void addErrorPage(int code, const std::string &path);
	const std::map<int, std::string> &getErrorPages() const;

	void addAllowedMethod(const std::string &method);
	const std::map<std::string, bool> &getAllowedMethods() const;
	bool isAllowedMethodsSet() const;

	void setAutoindex(bool autoindex);
	bool getAutoindex() const;
	bool isAutoindexSet() const;

	void addCgiBin(const std::string &ext, const std::string &cgiBin);
	const std::map<std::string, std::string> &getCgiBin() const;

	void setReturnDirective(const std::string &statusCode, const std::string &ret);
	const std::pair<std::string, std::string> &getReturnDirective() const;
	bool isReturnDirectiveSet() const;

	void addLocation(Location *loc);

	// Search for a Location based on the requested URI (longest prefix match).
	// Returns a pointer to the matching Location, or NULL if none found.
	Location *getLocationForURI(const std::string &uri) const;
	std::vector<Location *> getLocations() const;

private:
	std::set<std::string> _listens;						  // e.g., "0.0.0.0:80", "localhost:90", etc.
	std::set<std::string> _serverNames;					  // e.g., "example.com", "www.example.com"
	std::string _root;									  // Default: "/var/www/html"
	std::set<std::string> _index;						  // Default: "index.html"
	std::map<int, std::string> _errorPages;				  // e.g., 404->"/404.html", 500/502/503/504->"/50x.html"
	std::map<std::string, bool> _allowedMethods;		  // Default: GET, POST, DELETE
	bool _autoindex;									  // Default: off (false)
	std::map<std::string, std::string> _cgiBin;			  // Maps file extensions to CGI executables (e.g., ".pl" -> "/usr/bin/perl")
	std::pair<std::string, std::string> _returnDirective; // e.g., <"301": "http://example.com/default">
	LocationTrie _locationTrie;							  // Trie for storing Location blocks

	// Flags to indicate whether each optional field was explicitly set.
	bool _listensSet;
	bool _serverNamesSet;
	bool _rootSet;
	bool _indexSet;
	std::set<int> _errorPagesSet;
	bool _allowedMethodsSet;
	bool _autoindexSet;
	bool _returnDirectiveSet;
};
