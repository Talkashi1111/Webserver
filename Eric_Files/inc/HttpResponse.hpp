// This class is responsible for handling HTTP responses.
// It is a subclass of AHttpMessage.
//
// It has three private attributes:
// _version: a string representing the HTTP version (e.g., HTTP/1.1).
// _status: an integer representing the status code of the response (e.g., 200, 404, 500).
// _reason: a string representing the reason phrase of the response (e.g., OK, Not Found, Internal Server Error).

#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

# include "AHttpMessage.hpp"

class HttpResponse : public AHttpMessage
{
    private:
        // Private Attributes
        std::string _version;
        int         _status;
        std::string _reason;

        // Private Methods
        void                validateVersion(const std::string &version);
        void                validateStatus(const int status);
        void                validateReason(const std::string &reason);

        void                parseStartLine(const std::string &line);

    public:
        // Constructors and destructor
        HttpResponse();
        HttpResponse(const HttpResponse &src);
        HttpResponse &operator=(const HttpResponse &src);
        ~HttpResponse();

        // Setters
        void                setVersion(const std::string &version);
        void                setStatus(const int status);
        void                setReason(const std::string &reason);

        // Getters
        const std::string   &getVersion() const;
        int                 getStatus() const;
        const std::string   &getReason() const;

        // Public Methods
        void                parse(const std::string &raw);
        std::string         serialize() const;

        // Exceptions
        class InvalidVersion : public std::exception
        {
            public:
                virtual const char *what() const throw()
                {
                    return "Invalid version";
                }
        };
        
        class InvalidStatus : public std::exception
        {
            public:
                virtual const char *what() const throw()
                {
                    return "Invalid status";
                }
        };

        class InvalidReason : public std::exception
        {
            public:
                virtual const char *what() const throw()
                {
                    return "Invalid reason";
                }
        };
};

#endif