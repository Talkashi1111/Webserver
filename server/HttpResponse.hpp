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

private:
	std::string _response;
};
