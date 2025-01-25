// This class is responsible for handling HTTP requests.
// It is a subclass of AHttpMessage.
//
// It has three private attributes:
// _method: a string representing the HTTP method (e.g., GET, POST, PUT, DELETE).
// _target: a string representing the target of the request (e.g., /, /index.html).
// _version: a string representing the HTTP version (e.g., HTTP/1.1).


#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include "AHttpMessage.hpp"

class HttpRequest : public AHttpMessage
{
    private:
        // Private Attributes
        std::string _method;
        std::string _target;
        std::string _version;

        // Private Methods
        void                validateMethod(const std::string &method);
        void                validateTarget(const std::string &target);
        void                validateVersion(const std::string &version);

        void                parseStartLine(const std::string &line);

    public:
        // Constructors and destructor
        HttpRequest();
        HttpRequest(const HttpRequest &src);
        HttpRequest &operator=(const HttpRequest &src);
        ~HttpRequest();

        // Setters
        void                setMethod(const std::string &method);
        void                setTarget(const std::string &target);
        void                setVersion(const std::string &version);

        // Getters
        const std::string   &getMethod() const;
        const std::string   &getTarget() const;
        const std::string   &getVersion() const;

        // Public Methods
        void                parse(const std::string &raw);
        std::string         serialize() const;

        // Exceptions
        class InvalidMethod : public std::exception
        {
            public:
                virtual const char *what() const throw()
                {
                    return "Invalid method";
                }
        };

        class InvalidTarget : public std::exception
        {
            public:
                virtual const char *what() const throw()
                {
                    return "Invalid target";
                }
        };

        class InvalidVersion : public std::exception
        {
            public:
                virtual const char *what() const throw()
                {
                    return "Invalid version";
                }
        };
};

#endif