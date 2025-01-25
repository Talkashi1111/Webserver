#include "../inc/HttpRequest.hpp"

// Constructors and destructor

HttpRequest::HttpRequest() : AHttpMessage()
{

}

HttpRequest::HttpRequest(const HttpRequest &src) : AHttpMessage()
{
    *this = src;
}

HttpRequest &HttpRequest::operator=(const HttpRequest &src)
{
    if (this != &src)
    {
        this->_headers = src._headers;
        this->_body = src._body;
        this->_method = src._method;
        this->_target = src._target;
        this->_version = src._version;
    }
    return *this;
}

HttpRequest::~HttpRequest() {}

// Setters

void    HttpRequest::setMethod(const std::string &method)
{
    this->_method = method;
}

void    HttpRequest::setTarget(const std::string &target)
{
    this->_target = target;
}

void    HttpRequest::setVersion(const std::string &version)
{
    this->_version = version;
}

// Getters

const std::string   &HttpRequest::getMethod() const
{
    return this->_method;
}

const std::string   &HttpRequest::getTarget() const
{
    return this->_target;
}

const std::string   &HttpRequest::getVersion() const
{
    return this->_version;
}

// Private Methods

void    HttpRequest::validateMethod(const std::string &method)
{
    if (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE")
        throw InvalidMethod();
}

void    HttpRequest::validateTarget(const std::string &target)
{
    if (target.empty())
        throw InvalidTarget();
}

void    HttpRequest::validateVersion(const std::string &version)
{
    if (version != "HTTP/1.1")
        throw InvalidVersion();
}

void    HttpRequest::parseStartLine(const std::string &line)
{
    std::istringstream  lineStream(line);
    std::string         method;
    std::string         target;
    std::string         version;

    lineStream >> method >> target >> version;

    this->validateMethod(method);
    this->validateTarget(target);
    this->validateVersion(version);

    this->setMethod(method);
    this->setTarget(target);
    this->setVersion(version);
}

// Public Methods

void    HttpRequest::parse(const std::string &raw)
{
    std::istringstream  stream(raw);
    std::string         line;

    // Parse the request line
    std::getline(stream, line);
    this->parseStartLine(line); 

    // Parse the headers
    while (std::getline(stream, line) && line != "\r")
        this->parseHeader(line);

    // Parse the body
    std::string body;
    while (std::getline(stream, line))
        body += line + "\n";
    this->setBody(body);
}

std::string HttpRequest::serialize() const
{
    std::ostringstream  stream;

    // Serialize the request line
    stream << this->getMethod() << " " << this->getTarget() << " " << this->getVersion() << "\r\n";

    // Serialize the headers
    std::map<std::string, std::string>  headers = this->getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); it++)
        stream << it->first << ": " << it->second << "\r\n";

    // Serialize the body
    stream << "\r\n" << this->getBody();

    return stream.str();   
}