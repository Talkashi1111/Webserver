#include "../inc/HttpResponse.hpp"

// Constructors and destructor

HttpResponse::HttpResponse() : AHttpMessage() {}

HttpResponse::HttpResponse(const HttpResponse &src) : AHttpMessage()
{
    *this = src;
}

HttpResponse    &HttpResponse::operator=(const HttpResponse &src)
{
    if (this != &src)
    {
        AHttpMessage::operator=(src);
        this->_version = src._version;
        this->_status = src._status;
        this->_reason = src._reason;
    }
    return (*this);
}

HttpResponse::~HttpResponse() {}

// Setters

void    HttpResponse::setVersion(const std::string &version)
{
	this->validateVersion(version);
    this->_version = version;
}

void    HttpResponse::setStatus(const int status)
{
	this->validateStatus(status);
    this->_status = status;
}

void    HttpResponse::setReason(const std::string &reason)
{
    this->_reason = reason;
}

// Getters

const std::string   &HttpResponse::getVersion() const
{
    return (this->_version);
}

int HttpResponse::getStatus() const
{
    return (this->_status);
}

const std::string   &HttpResponse::getReason() const
{
    return (this->_reason);
}

// Private Methods

void    HttpResponse::validateVersion(const std::string &version)
{
    if (version != "HTTP/1.1")
        throw InvalidResponse("Unsupported HTTP version: " + version);
}

void    HttpResponse::validateStatus(const int status)
{
    if (status < 100 || status > 599)
        throw InvalidResponse("Invalid status code: " + std::to_string(status));
}

void    HttpResponse::parseStartLine(const std::string &line)
{
    std::istringstream  lineStream(line);
    std::string         version;
    int                 status;
    std::string         reason;

	// Parse the first line
    if (!(lineStream >> version))
		throw InvalidResponse("Failed to read HTTP version");
	if (!(lineStream >> status))
		throw InvalidResponse("Failed to read HTTP status code");

	std::getline(lineStream, reason);
	// Remove the leading space if it exists
	if (!reason.empty() && reason[0] == ' ')
		reason = reason.erase(0, 1);

	// Set the version, status, and reason
    this->setVersion(trim(version));
    this->setStatus(status);
    this->setReason(reason);
}

// Public Methods

void    HttpResponse::parse(const std::string &raw)
{
	try
	{
    std::istringstream  stream(raw);
    std::string         line;

    // Parse the first line
    if (!std::getline(stream, line))
		throw InvalidResponse("Failed to read HTTP response line");

	// Remove the carriage return if it exists
	if (!line.empty() && line.back() == '\r')
		line.pop_back();

    this->parseStartLine(line);

    // Parse the headers
    while (std::getline(stream, line) && line != "\r")
	{
		// Remove the carriage return if it exists
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

        this->parseHeader(line);
	}

    // Parse the body
    std::string body;
    while (std::getline(stream, line))
        body += line + "\n";
    this->setBody(body);
	}
	catch (const std::exception &e)
	{
		throw InvalidResponse(std::string("Parsing Response error: ") + e.what());
	}
}

std::string HttpResponse::serialize() const
{
    std::ostringstream  stream;

    // Serialize the first line
    stream << this->getVersion() << " " << this->getStatus() << " " << this->getReason() << "\r\n";

    // Serialize the headers
    std::map<std::string, std::vector<std::string> >  headers = this->getHeaders();
    for (std::map<std::string, std::vector<std::string> >::const_iterator it = headers.begin(); it != headers.end(); it++)
    {
        std::string key = it->first;
        for (std::vector<std::string>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
            stream << key << ": " << *it2 << "\r\n";
    }

    // Serialize the body
    stream << "\r\n" << this->getBody();

    return stream.str();
}
