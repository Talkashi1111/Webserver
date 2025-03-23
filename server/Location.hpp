#pragma once

#include <set>
#include <string>
#include <vector>
#include <map>

class Location
{
public:
	Location();
	Location(const std::string &path);
	Location(const Location &other);
	Location &operator=(const Location &other);
	~Location();

	// Setters / Getters
	void setPath(const std::string &path);
	const std::string &getPath() const;

	void addAllowedMethod(const std::string &method);
	const std::map<std::string, bool> &getAllowedMethods() const;
	bool isAllowedMethodSet() const;

	void setRoot(const std::string &root);
	const std::string &getRoot() const;
	bool isRootSet() const;

	void addIndex(const std::string &indexFile);
	const std::set<std::string> &getIndex() const;
	bool isIndexSet() const;

	void setAutoindex(bool autoindex);
	bool getAutoindex() const;
	bool isAutoindexSet() const;

	void setReturnDirective(const std::string &ret);
	const std::string &getReturnDirective() const;
	bool isReturnDirectiveSet() const;

	void setUploadDirectory(const std::string &uploadDir);
	const std::string &getUploadDirectory() const;
	bool isUploadDirectorySet() const;

private:
	std::string _path;							 // The location's URI pattern (e.g., "/upload")
	std::map<std::string, bool> _allowedMethods; // Optional override for allowed methods
	std::string _root;							 // Optional override for document root in this location
	std::set<std::string> _index;				 // Optional override for index files
	bool _autoindex;							 // Override for autoindex (on/off)
	std::string _returnDirective;				 // Optional return directive (e.g., "302 http://example.com/special")
	std::string _uploadDirectory;				 // If this location handles uploads, the directory where files are saved

	// Flags to indicate whether each optional field was explicitly set.
	bool _allowedMethodsSet;
	bool _rootSet;
	bool _indexSet;
	bool _autoindexSet;
	bool _returnDirectiveSet;
	bool _uploadDirectorySet;
};
