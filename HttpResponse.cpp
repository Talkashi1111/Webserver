#include <sstream>
#include <fstream>
#include <iostream>
#include "HttpResponse.hpp"
#include "Consts.hpp"
#include "StringUtils.hpp"

HttpResponse::HttpResponse() : _response("")
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

void HttpResponse::appendResponse(const std::string &data)
{
	_response += data;
}

void HttpResponse::generateErrorResponse(const std::string &statusCode)
{
	std::string statusText = "Unknown Status Code";
	std::map<std::string, std::string>::const_iterator it = kStatusCodes.find(statusCode);
	if (it != kStatusCodes.end())
	{
		statusText = it->second;
	}
	std::string body =
		"<html>\n"
		"<head><title>" +
		statusCode + " " + statusText + "</title></head>\n"
										"<body>\n"
										"<center><h1>" +
		statusCode + " " + statusText + "</h1></center>\n"
										"<hr><center>webserver/1.0</center>\n"
										"</body>\n"
										"</html>\n";
	std::ostringstream oss;
	oss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
	oss << "Server: webserver/1.0\r\n";
	oss << "Date: " << getCurrentTime() << "\r\n";
	oss << "Content-Type: text/html\r\n";
	oss << "Content-Length: " << body.length() << "\r\n";
	oss << "Connection: close\r\n";
	oss << "\r\n"
		<< body;
	_response = oss.str();
}

void HttpResponse::generateErrorResponseFile(const std::string &statusCode, const std::string &filePath)
{
	// TODO: maybe we need to handle cgi here
	std::ifstream file(filePath.c_str());
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open file " << filePath << std::endl;
		generateErrorResponse(statusCode);
		return;
	}
	std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	std::string statusText = kStatusCodes.find(statusCode)->second;
	std::ostringstream oss;
	oss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
	oss << "Server: webserver/1.0\r\n";
	oss << "Date: " << getCurrentTime() << "\r\n";
	oss << "Content-Type: text/html\r\n";
	oss << "Content-Length: " << body.length() << "\r\n";
	oss << "Connection: close\r\n";
	oss << "\r\n"
		<< body;
	_response = oss.str();
}

