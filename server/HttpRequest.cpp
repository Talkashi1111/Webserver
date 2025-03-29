#include <cstdlib>		// for atoi
#include <iostream>
#include "HttpRequest.hpp"
#include "Consts.hpp"

HttpRequest::HttpRequest(int clientHeaderBufferSize) : _state(S_METHOD),
													   _method(""),
													   _target(""),
													   _query(""),
													   _version(""),
													   _headers(),
													   _body(""),
													   _headerLength(0),
													   _clientHeaderBufferSize(clientHeaderBufferSize),
													   _currentHeaderName(""),
													   _currentHeaderValue("")
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
												   _currentHeaderName(src._currentHeaderName),
												   _currentHeaderValue(src._currentHeaderValue)
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
		_currentHeaderName = src._currentHeaderName;
		_currentHeaderValue = src._currentHeaderValue;
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

//TODO: add reset state logic
void HttpRequest::parseRequest(const std::string &raw)
{
	for (std::size_t i = 0; i < raw.size(); i++)
	{
		char c = raw[i];
		switch (_state)
		{
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
		case S_HEADER_END:
			parseHeaderEnd(c);
			_headerLength++;
			break;
		case S_BODY:
			parseBody(c);
			break;
		case S_DONE:
			return;
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

void HttpRequest::parseMethod(char c)
{
	if (_method.length() > 6)
	{
		_state = S_ERROR;
		throw std::runtime_error("405");
	}
	// GET POST or DELETE
	else if (c == 'G' || c == 'E' || c == 'T' || c == 'P' || c == 'U' || c == 'D' || c == 'L' || c == 'O' || c == 'S')
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
}

void HttpRequest::parseSpacesBeforeUri(char c)
{
	if (c == ' ' || c == '\t') // some server accept \t as well
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
	if (c == ' ' || c == '\t') // some server accept \t as well
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
			_version[5] >= '0' && _version[5] <= '9' &&
			_version[6] == '.' &&
			_version[7] >= '0' && _version[7] <= '9')
			{
				throw std::runtime_error("505"); // HTTP Version Not Supported
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
	if (c == '\r')
	{
		_state = S_HEADER_END;
		return;
	}
	else if (c == ':')
	{
		_state = S_HEADER_COLON;
		return;
	}

	if (c < 32 || c >= 127)
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}

	_currentHeaderName += c;
}

void HttpRequest::parseHeaderColon(char c)
{
	if (c == ' ' || c == '\t') // some server accept \t as well
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
		_state = S_HEADER_END;
		_headers[_currentHeaderName] = _currentHeaderValue;
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

void HttpRequest::parseHeaderEnd(char c)
{
	if (c == '\n')
	{
		// Check if there is a body
		if (_headers.find("Content-Length") != _headers.end())
		{
			_state = S_BODY;
			return;
		}
		else if (_headers.find("Transfer-Encoding") != _headers.end())
		{
			_state = S_DONE;
			return;
		}
		_state = S_DONE;
		return;
	}

	_state = S_ERROR;
	throw std::runtime_error("400");
}

void HttpRequest::parseBody(char c)
{
	std::map<std::string, std::string>::const_iterator it = _headers.find("Content-Length");
	if (it == _headers.end())
	{
		_state = S_ERROR;
		throw std::runtime_error("411"); // Length Required
	}

	// Validate Content-Length is a valid number
	const std::string &lenStr = it->second;
	for (std::string::const_iterator ch = lenStr.begin(); ch != lenStr.end(); ++ch)
	{
		if (!isdigit(*ch))
		{
			_state = S_ERROR;
			throw std::runtime_error("400"); // Bad Request
		}
	}

	size_t contentLength;
	try
	{
		contentLength = static_cast<size_t>(std::atoi(lenStr.c_str()));
	}
	catch (...)
	{
		_state = S_ERROR;
		throw std::runtime_error("400");
	}

	// Check if body exceeds Content-Length
	if (_body.size() >= contentLength)
	{
		_state = S_ERROR;
		throw std::runtime_error("413"); // Content Too Large
	}

	_body += c;

	// Check if body is complete
	if (_body.size() == contentLength)
	{
		_state = S_DONE;
	}
}
