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
# define AHTTPMESSAGE_HPP

# include <string>
# include <map>
# include <iostream>
# include <sstream>

class AHttpMessage
{
    protected:
        // Protected Methods
        std::map<std::string, std::string>  _headers;
        std::string                         _body;

        // Protected Methods
        void                validateHeader(const std::string &key, const std::string &value)
        {
            if (key.empty() || value.empty())
                throw InvalidHeader();
        }
        void                parseHeader(const std::string &line)
        {
            std::istringstream  headerStream(line);
            std::string         key;
            std::string         value;

            std::getline(headerStream, key, ':');
            std::getline(headerStream, value);

            this->validateHeader(key, value);

            this->setHeader(key, value);
        }
    
    public:
        // Constructors and destructor
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
            return *this;
        }
        virtual ~AHttpMessage() {}

        // Setters
        void                setHeader(const std::string &key, const std::string &value)
        {
            this->_headers[key] = value;
        }
        void                setBody(const std::string &body)
        {
            this->_body = body;
        }

        // Getters
        const std::map<std::string, std::string> &getHeaders() const
        {
            return this->_headers;
        }
        const std::string   &getHeader(const std::string &key) const
        {
            return this->_headers.at(key);
        }
        const std::string   &getBody() const
        {
            return this->_body;
        }

        // Public Methods
        virtual void        parse(const std::string &raw) = 0;
        virtual std::string serialize() const = 0;

        // Exceptions
        class InvalidHeader : public std::exception
        {
            public:
                virtual const char *what() const throw()
                {
                    return "Invalid header line";
                }
        };
};

#endif