#include "../inc/HttpResponse.hpp"

// Constructors and destructor

HttpResponse::HttpResponse() : AHttpMessage()
{

}

HttpResponse::HttpResponse(const HttpResponse &src) : AHttpMessage()
{
    *this = src;
}

HttpResponse    &HttpResponse::operator=(const HttpResponse &src)
{
    if (this != &src)
    {
        this->_headers = src._headers;
        this->_body = src._body;
        this->_version = src._version;
        this->_status = src._status;
        this->_reason = src._reason;
    }
    return *this;
}

HttpResponse::~HttpResponse() {}

// Setters

void    HttpResponse::setVersion(const std::string &version)
{
    this->_version = version;
}

void    HttpResponse::setStatus(const int status)
{
    this->_status = status;
}

void    HttpResponse::setReason(const std::string &reason)
{
    this->_reason = reason;
}

// Getters

const std::string   &HttpResponse::getVersion() const
{
    return this->_version;
}

int HttpResponse::getStatus() const
{
    return this->_status;
}

const std::string   &HttpResponse::getReason() const
{
    return this->_reason;
}

// Private Methods

void    HttpResponse::validateVersion(const std::string &version)
{
    if (version != "HTTP/1.1")
        throw InvalidVersion();
}

void    HttpResponse::validateStatus(const int status)
{
    if (status < 100 || status > 599)
        throw InvalidStatus();
}

void    HttpResponse::validateReason(const std::string &reason)
{
    if (reason.empty())
        throw InvalidReason();
}

void    HttpResponse::parseStartLine(const std::string &line)
{
    std::istringstream  lineStream(line);
    std::string         version;
    int                 status;
    std::string         reason;

    lineStream >> version >> status >> reason;

    this->validateVersion(version);
    this->validateStatus(status);
    this->validateReason(reason);

    this->setVersion(version);
    this->setStatus(status);
    this->setReason(reason);
}

// Public Methods

void    HttpResponse::parse(const std::string &raw)
{
    std::istringstream  stream(raw);
    std::string         line;

    // Parse the first line
    std::getline(stream, line);
    this->parseStartLine(line);

    // Parse the headers
    while (std::getline(stream, line) && line != "\r")
        this->parseHeader(line);

    //Control last \r\n

    // Parse the body
    std::string body;
    while (std::getline(stream, line))
        body += line + "\n";
    this->setBody(body);
}

std::string HttpResponse::serialize() const
{
    std::ostringstream  stream;

    // Serialize the first line
    stream << this->getVersion() << " " << this->getStatus() << " " << this->getReason() << "\r\n";

    // Serialize the headers
    std::map<std::string, std::string>  headers = this->getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); it++)
        stream << it->first << ": " << it->second << "\r\n";

    // Serialize the body
    stream << "\r\n" << this->getBody();

    return stream.str();
}