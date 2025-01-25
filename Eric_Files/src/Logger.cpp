#include "Logger.hpp"

// Constructors and destructor

Logger::Logger() : _logToFile(false), _fileStream()
{
}

Logger::Logger(const std::string &logFile) : _logToFile(!logFile.empty()), _fileStream()
{
    if (this->_logToFile)
    {
        this->_fileStream.open(logFile.c_str(), std::ios::out | std::ios::app);
        if (!this->_fileStream.is_open())
            throw std::runtime_error("Failed to open log file: " + logFile);
    }
}

Logger::Logger(const Logger &src) : _logToFile(src._logToFile), _fileStream()
{
    *this = src;
}

Logger &Logger::operator=(const Logger &src)
{
    if (this != &src)
        this->_logToFile = src.getLogToFile();
    return *this;
}

Logger::~Logger()
{
    if (this->_logToFile)
        this->_fileStream.close();
}

// Setters

void    Logger::setLogToFile(bool logToFile)
{
    this->_logToFile = logToFile;
}

// Getters

bool    Logger::getLogToFile() const
{
    return this->_logToFile;
}


// Private Methods

std::string Logger::getCurrentDateTime()
{
    time_t      now = time(0);
    struct tm   *timeinfo = localtime(&now);
    char        buffer[80];

    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S]", timeinfo);
    return std::string(buffer);
}

// Public Methods

void    Logger::log(const std::string &message)
{
    if (this->_logToFile)
        this->_fileStream << this->getCurrentDateTime() << " " << message << std::endl;
    else
        std::cout << message << std::endl;
}