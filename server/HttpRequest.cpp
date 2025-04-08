#include <cstdlib> // for atoi
#include <iostream>
#include <stdexcept>
#include <sstream> // for stringstream
#include <climits> // for LONG_MAX
#include <cerrno>
#include "Globals.hpp"
#include "HttpRequest.hpp"
#include "Consts.hpp"
#include "StringUtils.hpp"

HttpRequest::HttpRequest(int clientHeaderBufferSize, int clientMaxBodySize) : _state(S_START),
																			  _method(""),
																			  _target(""),
																			  _query(""),
																			  _version(""),
																			  _headers(),
																			  _body(""),
																			  _headerLength(0),
																			  _clientHeaderBufferSize(clientHeaderBufferSize),
																			  _clientMaxBodySize(clientMaxBodySize),
																			  _currentHeaderName(""),
																			  _currentHeaderValue(""),
																			  _expectedBodyLength(0),
																			  _isChunked(false),
																			  _currentChunkSize(0),
																			  _currentChunkRead(0),
																			  _chunkSizeLine("")
{
}

HttpRequest::HttpRequest(const HttpRequest &src) : _state(src._state),
												   _method(src._method),
												   _target(src._target),
												   _query(src._query),
												   _version(src._version),
												   _headers(src._headers),
												   _body(src._body),
												   _headerLength(src._headerLength),
												   _clientHeaderBufferSize(src._clientHeaderBufferSize),
												   _clientMaxBodySize(src._clientMaxBodySize),
												   _currentHeaderName(src._currentHeaderName),
												   _currentHeaderValue(src._currentHeaderValue),
												   _expectedBodyLength(src._expectedBodyLength),
												   _isChunked(src._isChunked),
												   _currentChunkSize(src._currentChunkSize),
												   _currentChunkRead(src._currentChunkRead),
												   _chunkSizeLine(src._chunkSizeLine)
{
}

HttpRequest &HttpRequest::operator=(const HttpRequest &src)
{
	if (this != &src)
	{
		_state = src._state;
		_method = src._method;
		_target = src._target;
		_query = src._query;
		_version = src._version;
		_headers = src._headers;
		_body = src._body;
		_headerLength = src._headerLength;
		_clientHeaderBufferSize = src._clientHeaderBufferSize;
		_clientMaxBodySize = src._clientMaxBodySize;
		_currentHeaderName = src._currentHeaderName;
		_currentHeaderValue = src._currentHeaderValue;
		_expectedBodyLength = src._expectedBodyLength;
		_isChunked = src._isChunked;
		_currentChunkSize = src._currentChunkSize;
		_currentChunkRead = src._currentChunkRead;
		_chunkSizeLine = src._chunkSizeLine;
	}
	return *this;
}

HttpRequest::~HttpRequest()
{
}

RequestState HttpRequest::getState() const
{
	return _state;
}

void HttpRequest::parseRequest(const std::string &raw)
{
	for (std::size_t i = 0; i < raw.size(); i++)
	{
		char c = raw[i];
		switch (_state)
		{
		case S_START:
			parseStart(c);
			break;
		case S_RESTART:
			parseRestart(c);
			break;
		case S_METHOD:
			parseMethod(c);
			_headerLength++;
			break;
		case SP_BEFORE_URI:
			parseSpacesBeforeUri(c);
			_headerLength++;
			break;
		case S_URI:
			parseUri(c);
			_headerLength++;
			break;
		case S_QUERY:
			parseQuery(c);
			_headerLength++;
			break;
		case S_FRAGMENT:
			parseFragment(c);
			_headerLength++;
			break;
		case SP_BEFORE_VERSION:
			parseSpacesBeforeVersion(c);
			_headerLength++;
			break;
		case S_VERSION:
			parseVersion(c);
			_headerLength++;
			break;
		case S_REQUEST_LINE_END:
			parseRequestLineEnd(c);
			_headerLength++;
			break;
		case S_HEADER_NAME:
			parseHeaderName(c);
			_headerLength++;
			break;
		case S_HEADER_COLON:
			parseHeaderColon(c);
			_headerLength++;
			break;
		case S_HEADER_VALUE:
			parseHeaderValue(c);
			_headerLength++;
			break;
		case S_HEADER_CR:
			parseHeaderCR(c);
			_headerLength++;
			break;
		case S_HEADER_LF:
			parseHeaderLF(c);
			_headerLength++;
			break;
		case S_HEADER_END:
			parseHeaderEnd(c);
			_headerLength++;
			break;
		case S_HEX:
			parseHex(c);
			break;
		case S_HEX_END:
			parseHexEnd(c);
			break;
		case S_CHUNK:
			parseChunk(c);
			break;
		case S_CHUNK_END:
			parseChunkEnd(c);
			break;
		case S_BODY:
			parseBody(c);
			break;
		case S_BODY_END:
			parseBodyEnd(c);
			break;
		case S_BODY_LF:
			parseBodyLF(c);
			break;
		case S_MESSAGE_END:
			parseMessageEnd(c);
			break;
		case S_DONE:
			printRequestDBG();
			return; // TODO: should we just return here?
		case S_ERROR:
		default:
			throw std::runtime_error("400");
		}
		// check if the header length is greater than the configured max
		if (_headerLength > _clientHeaderBufferSize)
		{
			_state = S_ERROR;
			throw std::runtime_error("413");
		}
	}
}

void HttpRequest::parseStart(char c)
{
	if (c == '\r')
	{
		_state = S_RESTART;
		return;
	}
	else if (c == 'G' || c == 'P' || c == 'D') // GET POST DELETE
	{
		_headerLength++;
		_method += c;
		_state = S_METHOD;
		return;
	}
	else
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
}

void HttpRequest::parseRestart(char c)
{
	if (c == '\n')
	{
		_state = S_START;
		return;
	}
	_state = S_ERROR;
	throw std::runtime_error("400");
}

void HttpRequest::parseMethod(char c)
{
	if (_method.length() > 6)
	{
		_state = S_ERROR;
		throw std::runtime_error("405");
	}
	// GET POST or DELETE
	else if (c == 'E' || c == 'T' || c == 'U' || c == 'L' || c == 'O' || c == 'S')
	{
		_method += c;
	}
	else if (c == ' ')
	{
		if (_method == "GET" || _method == "POST" || _method == "DELETE")
			_state = SP_BEFORE_URI;
		else
		{
			_state = S_ERROR;
			throw std::runtime_error("405");
		}
	}
	else
	{
		_state = S_ERROR;
		throw std::runtime_error("405");
	}
}

void HttpRequest::parseSpacesBeforeUri(char c)
{
	if (c == ' ')
		return;

	if (c == '/')
	{
		_target += c;
		_state = S_URI;
	}
	else
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
}

void HttpRequest::parseUri(char c)
{
	if (c < 32 || c >= 127)
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
	else if (c == ' ')
		_state = SP_BEFORE_VERSION;
	else if (c == '?')
		_state = S_QUERY;
	else if (c == '#')
		_state = S_FRAGMENT;
	else
		_target += c;
}

void HttpRequest::parseQuery(char c)
{
	if (c < 32 || c >= 127)
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
	else if (c == ' ')
		_state = SP_BEFORE_VERSION;
	else if (c == '#')
		_state = S_FRAGMENT;
	else
		_query += c;
}

void HttpRequest::parseFragment(char c)
{
	if (c < 32 || c >= 127)
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
	else if (c == ' ')
		_state = SP_BEFORE_VERSION;
	// No need to store fragment since it is not used by the server
}

void HttpRequest::parseSpacesBeforeVersion(char c)
{
	if (c == ' ')
		return;
	else if (c == 'H')
	{
		_version += c;
		_state = S_VERSION;
	}
	else
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
}

void HttpRequest::parseVersion(char c)
{
	// Only valid format is "HTTP/1.1"
	if (c == '\r')
	{
		if (_version != "HTTP/1.1")
		{
			_state = S_ERROR;
			// If it's a valid HTTP version format but not 1.1
			if (_version.size() == 8 &&
				_version.substr(0, 5) == "HTTP/" &&
				_version[5] >= '1' && _version[5] <= '9' &&
				_version[6] == '.' &&
				_version[7] >= '0' && _version[7] <= '9')
			{
				throw std::runtime_error("505"); // HTTP Version Not Supported
												 // in real nginx, it will accept anything that start at 1.0 but bad request from 0.0 and accept anything after 1.0. from 2.0 method not supported
			}
			throw std::runtime_error("400"); // Bad Request
		}
		_state = S_REQUEST_LINE_END;
		return;
	}

	// Check for valid characters in HTTP version
	if (_version.size() >= 8)
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}

	// Build version string
	if ((c >= 'A' && c <= 'Z') || c == '/' || c == '.' ||
		(c >= '0' && c <= '9'))
	{
		_version += c;
	}
	else
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
}

void HttpRequest::parseRequestLineEnd(char c)
{
	if (c == '\n')
		_state = S_HEADER_NAME;
	else
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
}

void HttpRequest::parseHeaderName(char c)
{
	if (c == ':')
	{
		if (_currentHeaderName.empty())
		{
			_state = S_ERROR;
			throw std::runtime_error("400");
		}

		_state = S_HEADER_COLON;
		return;
	}

	if (!validHttpRequestChar(c) && !std::isspace(c)) // TODO: a-z , 0-9 and tchar
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}

	if (c >= 'A' && c <= 'Z')
		_currentHeaderName += c + 32; // convert to lower case
	else
		_currentHeaderName += c;
}

void HttpRequest::parseHeaderColon(char c)
{
	if (c == ' ' || c == '\t')
		return;

	if (c < 32 || c >= 127)
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}

	_state = S_HEADER_VALUE;
	_currentHeaderValue += c;
}

void HttpRequest::parseHeaderValue(char c)
{
	if (c == '\r')
	{
		_state = S_HEADER_CR;
		if (_headers.find("host") != _headers.end() && _currentHeaderName == "host")
		{
			_state = S_ERROR;
			throw std::runtime_error("400");
		} // TODO: handle additional duplicates

		_currentHeaderValue = trimFromEnd(_currentHeaderValue);
		if (_currentHeaderName == "transfer-encoding" && _headers.find("transfer-encoding") != _headers.end())
		{
			if (_currentHeaderValue == "chunked")
				_isChunked = true;
			_headers["transfer-encoding"] += ", " + _currentHeaderValue;
		}
		else if (_currentHeaderName == "connection")
		{
			if (_currentHeaderValue == "keep-alive")
				_headers["connection"] = "keep-alive";
			else if (_currentHeaderValue == "close")
				_headers["connection"] = "close";
		}
		else
		{
			_headers[_currentHeaderName] = _currentHeaderValue;
		}
		_currentHeaderName = "";
		_currentHeaderValue = "";
		return;
	}

	if (c < 32 || c >= 127)
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}

	_currentHeaderValue += c;
}

void HttpRequest::parseHeaderCR(char c)
{
	if (c == '\n')
	{
		_state = S_HEADER_LF;
		return;
	}

	_state = S_ERROR;
	throw std::runtime_error("400");
}

void HttpRequest::parseHeaderLF(char c)
{

	if (c == '\r')
	{
		if (_headers.find("host") == _headers.end())
		{
			_state = S_ERROR;
			throw std::runtime_error("400");
		}
		_state = S_HEADER_END;
		return;
	}
	else if (c <= 32 || c >= 127)
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
	_state = S_HEADER_NAME;
	if (validHttpRequestChar(c))
	{
		if (c >= 'A' && c <= 'Z')
			_currentHeaderName += c + 32; // convert to lower case
		else
			_currentHeaderName += c;
	}
	else
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
	return;
}

void HttpRequest::parseHeaderEnd(char c)
{
	if (c == '\n')
	{
		// Check if there is a body
		if (_headers.find("transfer-encoding") != _headers.end() &&
			_headers["transfer-encoding"].find("chunked") != std::string::npos)
		{
			_state = S_HEX;
			return;
		}
		else if (_headers.find("content-length") != _headers.end())
		{
			try
			{
				std::istringstream iss(_headers["content-length"]);
				iss >> _expectedBodyLength;
				if (iss.fail() || !iss.eof())
				{
					_state = S_ERROR;
					throw std::runtime_error("400");
				}
				else if (_expectedBodyLength > _clientMaxBodySize)
				{
					_state = S_ERROR;
					throw std::runtime_error("413");
				}
			}
			catch (std::invalid_argument &ia)
			{
				_state = S_ERROR;
				throw std::runtime_error("400");
			}
			_state = S_BODY;
			if (_expectedBodyLength == 0)
				_state = S_DONE;
			return;
		}
		else
		{
			_state = S_DONE;
			return;
		}
	}
	_state = S_ERROR;
	throw std::runtime_error("400");
}

void HttpRequest::parseHex(char c)
{
	if (c == '\r')
	{
		if (_chunkSizeLine.empty())
		{
			_state = S_ERROR;
			throw std::runtime_error("400");
		}
		_state = S_HEX_END;
		return;
	}
	if (_chunkSizeLine.length() > kMaxHexLength)
	{
		_state = S_ERROR;
		throw std::runtime_error("413"); // Entity too large
	}
	if (std::isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
		_chunkSizeLine += c;
	else
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}
}

void HttpRequest::parseHexEnd(char c)
{
	if (c == '\n')
	{
		// Extract and parse chunk size
		_currentChunkSize = std::strtoul(_chunkSizeLine.c_str(), NULL, 16);
		if (errno == ERANGE)
		{
			_state = S_ERROR;
			throw std::runtime_error("400"); // Invalid hex
		}
		if (_currentChunkSize > _clientMaxBodySize - _body.size())
		{
			_state = S_ERROR;
			throw std::runtime_error("413");
		}
		_chunkSizeLine.clear();
		_currentChunkRead = 0;
		_state = (_currentChunkSize == 0) ? S_BODY_LF : S_CHUNK;
		return;
	}

	_state = S_ERROR;
	throw std::runtime_error("400");
}

void HttpRequest::parseChunk(char c)
{
	if (_currentChunkRead == _currentChunkSize && c == '\r')
	{
		_state = S_CHUNK_END;
		return;
	}

	if (_currentChunkRead < _currentChunkSize)
	{
		_currentChunkRead++;
		_body += c;
		if (_body.size() > _clientMaxBodySize)
		{
			_state = S_ERROR;
			throw std::runtime_error("413");
		}
		return;
	}
	_state = S_ERROR;
	throw std::runtime_error("400");
}

void HttpRequest::parseChunkEnd(char c)
{
	if (c == '\n')
	{
		_state = S_HEX;
		return;
	}
	_state = S_ERROR;
	throw std::runtime_error("400");
}

void HttpRequest::parseBody(char c)
{
	if (c == '\r')
	{
		if (_body.size() == _expectedBodyLength)
			_state = S_BODY_END;
		return;
	}
	if (_body.size() >= _clientMaxBodySize)
	{
		_state = S_ERROR;
		throw std::runtime_error("413");
	}
	_body += c;
}

void HttpRequest::parseBodyEnd(char c)
{
	if (c == '\n')
	{
		_state = S_BODY_LF;
		return;
	}
	_state = S_ERROR;
	throw std::runtime_error("400");
}

void HttpRequest::parseBodyLF(char c)
{
	if (c == '\r')
	{
		_state = S_MESSAGE_END;
		return;
	}
	_state = S_ERROR;
	throw std::runtime_error("400");
}

void HttpRequest::parseMessageEnd(char c)
{
	if (c == '\n')
	{
		_state = S_DONE;
		return;
	}
	_state = S_ERROR;
	throw std::runtime_error("400");
}
void HttpRequest::printRequestDBG() const
{
	if (!DEBUG)
		return;
	std::cout << "Method: " << _method << std::endl;
	std::cout << "Target: " << _target << std::endl;
	std::cout << "Query: " << _query << std::endl;
	std::cout << "Version: " << _version << std::endl;
	std::cout << "Headers:" << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		std::cout << "  " << it->first << ": " << it->second << std::endl;
	}
	std::cout << "Body: " << _body << std::endl;
}

const std::string &HttpRequest::getHostName() const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find("host");
	if (it != _headers.end())
		return it->second;
	else
		return kDefaultServerName;
}

const std::string &HttpRequest::getTarget() const
{
	return _target;
}

bool HttpRequest::isKeepAlive() const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find("connection");
	if (it != _headers.end())
	{
		if (it->second == "close")
			return false;
		else if (it->second == "keep-alive")
			return true;
	}
	return kDefaultKeepAlive;
}

const std::string &HttpRequest::getVersion() const
{
	return _version;
}

const std::string &HttpRequest::getMethod() const
{
	return _method;
}

const std::string &HttpRequest::getBody() const
{
	return _body;
}

const std::string &HttpRequest::getQuery() const
{
	return _query;
}

const std::map<std::string, std::string> &HttpRequest::getHeaders() const
{
	return _headers;
}

const std::string &HttpRequest::getHeaderValue(const std::string key) const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it != _headers.end())
		return it->second;
	throw std::runtime_error("Header not found");
}
