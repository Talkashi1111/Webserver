#pragma once

#include <string>
#include <map>
#include "Consts.hpp"

class Server;
class Location;

class HttpRequest
{
public:
	HttpRequest(int clientHeaderBufferSize);
	HttpRequest(const HttpRequest &src);
	HttpRequest &operator=(const HttpRequest &src);
	~HttpRequest();

	// // Setters
	// void setMethod(const std::string &method);
	// void setTarget(const std::string &target);
	// void setVersion(const std::string &version); // only supported 1.1 if not return 505 HTTP Version Not Supported
	// void setHeader(const std::string &key, const std::string &value);
	// void setBody(const std::string &body);

	// // Getters
	// std::string getMethod() const;
	// std::string getTarget() const;
	// std::string getVersion() const;
	// std::string getHeader(const std::string &key) const;
	// std::string getBody() const;
	RequestState getState() const;

	void parseRequest(const std::string &raw);

private:
	RequestState _state;
	std::string _method;
	std::string _target;
	std::string _query;
	std::string _version;
	std::map<std::string, std::string> _headers;
	std::string _body;
	int _headerLength;
	int _clientHeaderBufferSize;
	std::string _currentHeaderName;
	std::string _currentHeaderValue;

	// parsing functions
	void parseMethod(char c);
	void parseSpacesBeforeUri(char c);
	void parseUri(char c);
	void parseQuery(char c);
	void parseFragment(char c);
	void parseSpacesBeforeVersion(char c);
	void parseVersion(char c);
	void parseRequestLineEnd(char c);
	void parseHeaderName(char c);
	void parseHeaderColon(char c);
	void parseHeaderValue(char c);
	void parseHeaderEnd(char c);
	void parseBody(char c);
};
