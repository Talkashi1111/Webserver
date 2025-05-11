#pragma once

#include <string>
#include <map>
#include "Consts.hpp"

class Server;
class Location;

class HttpRequest
{
public:
	HttpRequest(int clientHeaderBufferSize, int clientMaxBodySize);
	HttpRequest(const HttpRequest &src);
	HttpRequest &operator=(const HttpRequest &src);
	~HttpRequest();

	// // Getters
	RequestState getState() const;
	const std::string &getHostName() const;
	const std::string &getTarget() const;
	const std::string &getVersion() const;
	const std::string &getMethod() const;
	const std::string &getBody() const;
	const std::string &getQuery() const;
	const std::map<std::string, std::string> &getHeaders() const;
	const std::string &getHeaderValue(const std::string key) const;
	bool isKeepAlive() const;

	// // Setters
	void setState(RequestState state);

	void parseRequest(const std::string &raw);
	void printRequestDBG() const;

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
	size_t _clientMaxBodySize; // TODO: check that we enforce this limit
	std::string _currentHeaderName;
	std::string _currentHeaderValue;
	size_t _expectedBodyLength;
	bool _isChunked; // true if transfer-encoding is chunked

	// Chunked encoding variables
	size_t _currentChunkSize;	 // Size of current chunk being processed
    size_t _currentChunkRead;    // How many bytes read in current chunk
    std::string _chunkSizeLine;  // Buffer for partial chunk size line

	// parsing functions
	void parseStart(unsigned char c);
	void parseRestart(unsigned char c);
	void parseMethod(unsigned char c);
	void parseSpacesBeforeUri(unsigned char c);
	void parseUri(unsigned char c);
	void parseQuery(unsigned char c);
	void parseFragment(unsigned char c);
	void parseSpacesBeforeVersion(unsigned char c);
	void parseVersion(unsigned char c);
	void parseRequestLineEnd(unsigned char c);
	void parseHeaderName(unsigned char c);
	void parseHeaderColon(unsigned char c);
	void parseHeaderValue(unsigned char c);
	void parseHeaderCR(unsigned char c);
	void parseHeaderLF(unsigned char c);
	void parseHeaderEnd(unsigned char c);
	void parseHex(unsigned char c);
	void parseHexEnd(unsigned char c);
	void parseChunk(unsigned char c);
	void parseChunkEnd(unsigned char c);
	void parseBody(unsigned char c);
};
