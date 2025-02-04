#include "../inc/HttpRequest.hpp"

// Constructors and destructor

HttpRequest::HttpRequest() : AHttpMessage() {}

HttpRequest::HttpRequest(const HttpRequest &src) : AHttpMessage()
{
    *this = src;
}

HttpRequest &HttpRequest::operator=(const HttpRequest &src)
{
    if (this != &src)
    {
        AHttpMessage::operator=(src);
        this->_method = src._method;
        this->_target = src._target;
        this->_version = src._version;
    }
    return (*this);
}

HttpRequest::~HttpRequest() {}

// Setters

void    HttpRequest::setMethod(const std::string &method)
{
	this->validateMethod(method);
    this->_method = method;
}

void    HttpRequest::setTarget(const std::string &target)
{
	this->validateTarget(target);
    this->_target = target;
}

void    HttpRequest::setVersion(const std::string &version)
{
	this->validateVersion(version);
    this->_version = version;
}

// Getters

const std::string   &HttpRequest::getMethod() const
{
    return (this->_method);
}

const std::string   &HttpRequest::getTarget() const
{
    return (this->_target);
}

const std::string   &HttpRequest::getVersion() const
{
    return (this->_version);
}

// Private Methods

void    HttpRequest::validateMethod(const std::string &method)
{
    if (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE")
        throw InvalidRequest("Unsupported HTTP method: " + method);
}

void    HttpRequest::validateTarget(const std::string &target)
{
    if (target.empty())
        throw InvalidRequest("Empty request target");
}

void    HttpRequest::validateVersion(const std::string &version)
{
    if (version != "HTTP/1.1")
        throw InvalidRequest("Unsupported HTTP version: " + version);
}

void    HttpRequest::parseStartLine(const std::string &line)
{
    std::istringstream  lineStream(line);
    std::string         method;
    std::string         target;
    std::string         version;

	// Parse the method, target, and version
    if (!(lineStream >> method))
		throw InvalidRequest("Failed to parse request method");
	if (!(lineStream >> target))
		throw InvalidRequest("Failed to parse request target");
	if (!(lineStream >> version))
		throw InvalidRequest("Failed to parse request version");

	// Set the method, target, and version
    this->setMethod(trim(method));
    this->setTarget(trim(target));
    this->setVersion(trim(version));
}

// Public Methods

void    HttpRequest::parse(const std::string &raw)
{
	try
    {
		std::istringstream  stream(raw);
		std::string         line;

		// Parse the request line
		if (!std::getline(stream, line))
			throw InvalidRequest("Failed to parse request line");

		// Remove the carriage return if it exists
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		this->parseStartLine(line);

		// Parse the headers
		while (std::getline(stream, line) && line != "\r")
		{
			// Remove the carriage return if it exists
			if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

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
		throw InvalidRequest(std::string("Parsing Request error: ") + e.what());
	}
}

std::string HttpRequest::serialize() const
{
    std::ostringstream  stream;

    // Serialize the request line
    stream << this->getMethod() << " " << this->getTarget() << " " << this->getVersion() << "\r\n";

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
