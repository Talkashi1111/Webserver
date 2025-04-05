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
	bool isKeepAlive() const;
	const std::map<std::string, std::string> &getHeaders() const;

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
	void parseStart(char c);
	void parseRestart(char c);
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
	void parseHeaderCR(char c);
	void parseHeaderLF(char c);
	void parseHeaderEnd(char c);
	void parseHex(char c);
	void parseHexEnd(char c);
	void parseChunk(char c);
	void parseChunkEnd(char c);
	void parseBody(char c);
	void parseBodyEnd(char c);
	void parseBodyLF(char c);
	void parseMessageEnd(char c);
};
