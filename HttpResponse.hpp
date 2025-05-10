#pragma once

#include <string>

class HttpResponse
{
public:
	HttpResponse();
	HttpResponse(const HttpResponse &src);
	HttpResponse &operator=(const HttpResponse &src);
	~HttpResponse();

	void setResponse(const std::string &response);
	const std::string &getResponse() const;
	void eraseResponse(int nbytes);
	void generateErrorResponse(const std::string &statusCode);
	void generateErrorResponseFile(const std::string &statusCode, const std::string &filePath);
	void appendResponse(const std::string &data); // New method to append data to the response

private:
	std::string _response;
};
