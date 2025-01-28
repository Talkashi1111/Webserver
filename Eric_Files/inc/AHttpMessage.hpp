// File: AHttpMessage.hpp
//
// Abstract class that represents an HTTP message.
// It has pure virtual methods to parse and serialize the message.
// It is the base class for HttpRequest and HttpResponse.
//
// It has two protected attributes:
// _headers: a map of strings representing the headers of the message.
// _body: a string representing the body of the message.

#ifndef AHTTPMESSAGE_HPP
#define AHTTPMESSAGE_HPP

#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <iostream>

class AHttpMessage
{
protected:
    std::map<std::string, std::vector<std::string> > _headers;
    std::string _body;

    static std::string trim(const std::string &s)
    {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
            start++;

        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
            end--;

        return (s.substr(start, end - start));
    }

    static std::string toLower(const std::string &s)
    {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
        return (result);
    }

    void validateHeader(const std::string &key)
    {
        if (key.empty()) {
            throw InvalidHeader();
        }
    }

    void parseHeader(const std::string &line)
    {
        std::istringstream headerStream(line);
        std::string key, value;

        if (!std::getline(headerStream, key, ':'))
            throw InvalidHeader();
        if (!std::getline(headerStream, value))
            value = "";

        key   = trim(key);
        value = trim(value);

        validateHeader(key);

        std::string lowerKey = toLower(key);

        this->_headers[lowerKey].push_back(value);
    }

public:
    // Constructors / destructor

    AHttpMessage() {}

    AHttpMessage(const AHttpMessage &src)
    {
        *this = src;
    }

    AHttpMessage &operator=(const AHttpMessage &src)
    {
        if (this != &src)
        {
            this->_headers = src._headers;
            this->_body = src._body;
        }
        return (*this);
    }

    virtual ~AHttpMessage() {}

    // Setters

    void addHeader(const std::string &key, const std::string &value)
    {
        std::string lowerKey = toLower(trim(key));
        std::string val = trim(value);

        validateHeader(lowerKey);
        this->_headers[lowerKey].push_back(val);
    }

    void setHeader(const std::string &key, const std::string &value)
    {
        std::string lowerKey = toLower(trim(key));
        std::string val = trim(value);

        validateHeader(lowerKey);
        this->_headers[lowerKey].clear();
        this->_headers[lowerKey].push_back(val);
    }

    void setBody(const std::string &body)
    {
        this->_body = body;
    }

    // Getters

    const std::map<std::string, std::vector<std::string> > &getHeaders() const
    {
        return (this->_headers);
    }

    bool hasHeader(const std::string &key) const
    {
        std::string lowerKey = toLower(trim(key));
        return (this->_headers.find(lowerKey) != this->_headers.end());
    }

    const std::string &getHeader(const std::string &key) const
    {
        std::string lowerKey = toLower(trim(key));
        return (this->_headers.at(lowerKey).at(0));
    }

    const std::vector<std::string> &getHeaderValues(const std::string &key) const
    {
        std::string lowerKey = toLower(trim(key));
        return (this->_headers.at(lowerKey));
    }

    std::string getHeaderOrDefault(const std::string &key, const std::string &defaultValue) const
    {
        std::string lowerKey = toLower(trim(key));
        std::map<std::string, std::vector<std::string> >::const_iterator it =
            this->_headers.find(lowerKey);

        if (it == this->_headers.end() || it->second.empty()) {
            return (defaultValue); // key not found or no values
        }
        return (it->second[0]);
    }

    const std::string &getBody() const
    {
        return (this->_body);
    }

    virtual void parse(const std::string &raw) = 0;
    virtual std::string serialize() const = 0;

    // Exceptions
    class InvalidHeader : public std::exception
    {
    public:
        virtual const char* what() const throw()
        {
            return ("Invalid header line");
        }
    };
};

#endif // AHTTPMESSAGE_HPP
