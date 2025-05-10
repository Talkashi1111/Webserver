#pragma once

#include <map>
#include <vector>
#include <string>
#include <utility> // for std::pair, std::make_pair

enum RequestState
{
	// request line parsing
	S_START,
	S_RESTART,
	S_METHOD,
	SP_BEFORE_URI,
	S_URI,
	S_QUERY,
	S_FRAGMENT,
	SP_BEFORE_VERSION,
	S_VERSION,
	S_REQUEST_LINE_END, //\r\n
	// header parsing
	S_HEADER_NAME,
	S_HEADER_COLON,
	S_HEADER_VALUE,
	S_HEADER_CR, //\r\n
	S_HEADER_LF,
	S_HEADER_END, //\r\n\r\n
	// chunked encoding parsing
	S_HEX,         // Reading chunk size
    S_HEX_END,         // Reading chunk data
    S_CHUNK,
    S_CHUNK_END,
	// body parsing
	S_BODY,

	S_DONE, // for reading Content-Length (body) or Transfer-Encoding (chunked)
	S_ERROR,

	S_CGI_PROCESSING,
};

extern const std::string kDefaultConfig;
extern const int kMaxBuff;
extern const std::string kDefaultListen;
extern const std::string kDefaultHost;
extern const std::string kDefaultPort;
extern const std::string kDefaultServerName;
extern const std::string kDefaultRoot;
extern const int kDefaultClientTimeout;
extern const int kDefaultClientHeaderBufferSize;
extern const size_t kDefaultClientMaxBodySize;
extern const std::map<std::string, bool> kDefaultAllowedMethods;
extern const std::vector<std::string> kDefaultIndex;
extern const std::map<int, std::string> kDefaultErrorPages;
extern const bool kDefaultAutoindex;
extern const std::map<std::string, std::string> kStatusCodes;
extern const size_t kMaxHexLength;
extern const bool kDefaultKeepAlive;
