// This class is responsible for handling logging.
// It is used to log messages to the console and/or to a file.
//
// It has two private attributes:
// _logToFile: a boolean indicating whether the log should be written to a file.
// _fileStream: an ofstream object used to write to the log file.

#ifndef LOGGER_HPP
# define LOGGER_HPP

# include <string>
# include <fstream>
# include <iostream>

class Logger
{
    private:
        // Private Attributes
        bool            _logToFile;
        std::ofstream   _fileStream;

        // Private Methods
        std::string     getCurrentDateTime();

    public:
        // Constructors and destructor
        Logger();
        Logger(const std::string &logFile);
        Logger(const Logger &src);
        Logger &operator=(const Logger &src);
        ~Logger();

        // Setters
        void            setLogToFile(bool logToFile);

        // Getters
        bool            getLogToFile() const;

        // Public Methods
        void            log(const std::string &message);
};

#endif