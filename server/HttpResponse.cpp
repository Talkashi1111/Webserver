#include "HttpResponse.hpp"

HttpResponse::HttpResponse() : _response("HTTP/1.1 200 OK\r\n")
{
}

HttpResponse::HttpResponse(const HttpResponse &src) : _response(src._response)
{
}

HttpResponse &HttpResponse::operator=(const HttpResponse &src)
{
	if (this != &src)
	{
		_response = src._response;
	}
	return *this;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::setResponse(const std::string &response)
{
	_response = response;
}

const std::string &HttpResponse::getResponse() const
{
	return _response;
}

void HttpResponse::eraseResponse(int nbytes)
{
	_response.erase(0, nbytes);
}
