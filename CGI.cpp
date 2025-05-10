#include <csignal>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "CGI.hpp"
#include "ProcUtils.hpp"
#include "FileUtils.hpp"
#include "HttpRequest.hpp"
#include "StringUtils.hpp"
#include "Globals.hpp"

CGI::CGI() : _inFd(-1), _outFd(-1), _pid(-1), _cgiBodySentBytes(0)
{
}

CGI::CGI(const CGI &other) : _inFd(other._inFd),
							 _outFd(other._outFd),
							 _pid(other._pid),
							 _cgiBodySentBytes(other._cgiBodySentBytes)
{
}

CGI &CGI::operator=(const CGI &other)
{
	if (this != &other)
	{
		_inFd = other._inFd;
		_outFd = other._outFd;
		_pid = other._pid;
		_cgiBodySentBytes = other._cgiBodySentBytes;
	}
	return *this;
}

CGI::~CGI()
{
	reset();
}

void CGI::reset()
{
	if (_inFd != -1)
	{
		closeFd(_inFd);
		_inFd = -1;
	}
	if (_outFd != -1)
	{
		closeFd(_outFd);
		_outFd = -1;
	}

	if (_pid != -1)
	{
		if (kill(_pid, SIGTERM) == -1)
		{
			if (DEBUG)
				std::cerr << _pid << ": kill SIGTERM: " << strerror(errno) << std::endl;
		}
		else
		{
			doWaitpid(_pid, WNOHANG);
		}
	}
	_pid = -1;
	_cgiBodySentBytes = 0;
}

int CGI::getInFd() const
{
	return _inFd;
}

void CGI::setInFd(int fd)
{
	_inFd = fd;
}

int CGI::getOutFd() const
{
	return _outFd;
}

void CGI::setOutFd(int fd)
{
	_outFd = fd;
}

pid_t CGI::getPid() const
{
	return _pid;
}

void CGI::setPid(pid_t pid)
{
	_pid = pid;
}

int CGI::getCgiBodySentBytes() const
{
	return _cgiBodySentBytes;
}

void CGI::setCgiBodySentBytes(int bytes)
{
	_cgiBodySentBytes = bytes;
}

char **CGI::createEnv(const HttpRequest &request, const std::string &scriptPath,
					  const std::string &localPort, const std::string &remoteHost) const
{
	std::vector<std::string> env_strings;

	env_strings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_strings.push_back("SERVER_PROTOCOL=" + request.getVersion());
	env_strings.push_back("SERVER_SOFTWARE=webserver/1.0");
	env_strings.push_back("REQUEST_METHOD=" + request.getMethod());
	env_strings.push_back("SCRIPT_FILENAME=" + scriptPath);
	env_strings.push_back("PATH_INFO=" + scriptPath);
	env_strings.push_back("PATH_TRANSLATED=" + scriptPath);
	env_strings.push_back("SCRIPT_NAME=" + request.getTarget());
	env_strings.push_back("REQUEST_URI=" + request.getTarget());
	env_strings.push_back("QUERY_STRING=" + request.getQuery());
	env_strings.push_back("SERVER_NAME=" + request.getHostName());
	env_strings.push_back("SERVER_PORT=" + localPort);
	env_strings.push_back("REMOTE_ADDR=" + remoteHost);
	env_strings.push_back("REMOTE_HOST=" + remoteHost);

	// Headers as environment variables
	const std::map<std::string, std::string> &headers = request.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		std::string header_name = it->first;
		// Convert header to CGI format (HTTP_HEADER_NAME)
		for (size_t i = 0; i < header_name.length(); i++)
		{
			if (header_name[i] == '-')
				header_name[i] = '_';
			else if (header_name[i] >= 'a' && header_name[i] <= 'z')
				header_name[i] = header_name[i] - 32; // Convert to uppercase
		}
		env_strings.push_back("HTTP_" + header_name + "=" + it->second);
	}

	// Content-related variables for POST requests
	if (request.getMethod() == "POST")
	{
		// Convert body length to string using the helper function
		env_strings.push_back("CONTENT_LENGTH=" + numberToString(request.getBody().length()));
		try
		{
			const std::string &content_type = request.getHeaderValue("content-type");
			env_strings.push_back("CONTENT_TYPE=" + content_type);
		}
		catch (const std::exception &e)
		{
		}
	}

	// Allocate space for environment array (null-terminated)
	char **envp = NULL;
	try
	{
		envp = new char *[env_strings.size() + 1];

		for (size_t i = 0; i < env_strings.size(); i++)
		{
			try
			{
				envp[i] = new char[env_strings[i].length() + 1];
				std::strcpy(envp[i], env_strings[i].c_str());
			}
			catch (const std::bad_alloc &e)
			{
				// Clean up already allocated memory if allocation fails
				for (size_t j = 0; j < i; j++)
				{
					delete[] envp[j];
				}
				delete[] envp;
				throw; // Re-throw to be caught by outer try-catch
			}
		}
		envp[env_strings.size()] = NULL;

		return envp;
	}
	catch (const std::bad_alloc &e)
	{
		std::cerr << "memory allocation failed in createEnv: " << e.what() << std::endl;
		return NULL;
	}
}

void CGI::start(const HttpRequest &request, const std::string &cgiPath, const std::string &scriptPath,
				const std::string &localPort, const std::string &remoteHost)
{
	// Check if CGI executable exists and is executable
	if (access(cgiPath.c_str(), X_OK) == -1)
	{
		int err = errno;
		if (err == ENOENT)
			throw std::runtime_error("404"); // Not found
		else if (err == EACCES)
			throw std::runtime_error("403"); // Forbidden
		else
			throw std::runtime_error("500"); // Internal Server Error
	}

	// Check if script file exists and is readable
	if (access(scriptPath.c_str(), R_OK) == -1)
	{
		int err = errno;
		if (err == ENOENT)
			throw std::runtime_error("404"); // Script not found
		else if (err == EACCES)
			throw std::runtime_error("403"); // Script not readable
		else
			throw std::runtime_error("500"); // Internal Server Error
	}

	int pipeIn[2];
	int pipeOut[2];

	if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1)
	{
		perror("pipe");
		throw std::runtime_error("500");
	}

	// Set parent process side of pipes to non-blocking
	if (setNonblocking(pipeIn[1]) == false || setNonblocking(pipeOut[0]) == false)
	{
		closeFd(pipeIn[0]);
		closeFd(pipeIn[1]);
		closeFd(pipeOut[0]);
		closeFd(pipeOut[1]);
		throw std::runtime_error("500");
	}

	pid_t pid = fork();
	if (pid == -1)
	{
		perror("fork");
		closeFd(pipeIn[0]);
		closeFd(pipeIn[1]);
		closeFd(pipeOut[0]);
		closeFd(pipeOut[1]);
		throw std::runtime_error("500");
	}
	else if (pid == 0) // Child process
	{
		closeFd(pipeIn[1]);	 // Close unused write end of pipeIn
		closeFd(pipeOut[0]); // Close unused read end of pipeOut
		if (dup2(pipeIn[0], STDIN_FILENO) == -1 || dup2(pipeOut[1], STDOUT_FILENO) == -1)
		{
			perror("dup2");
			closeFd(pipeIn[0]);	 // Close read end of pipeIn
			closeFd(pipeOut[1]); // Close write end of pipeOut
			exit(EXIT_FAILURE);
		}
		closeFd(pipeIn[0]);	 // Close read end of pipeIn
		closeFd(pipeOut[1]); // Close write end of pipeOut

		char **envp = NULL;
		char **argv = NULL;

		try
		{
			envp = createEnv(request, scriptPath, localPort, remoteHost);
			if (envp == NULL)
			{
				std::cerr << "failed to create environment variables" << std::endl;
				exit(EXIT_FAILURE);
			}

			argv = new char *[3];
			argv[0] = new char[cgiPath.length() + 1];
			std::strcpy(argv[0], cgiPath.c_str());
			argv[1] = new char[scriptPath.length() + 1];
			std::strcpy(argv[1], scriptPath.c_str());
			argv[2] = NULL;

			execve(cgiPath.c_str(), argv, envp);
			perror("execve");

			// Clean up allocated memory
			for (char **env = envp; *env != NULL; env++)
				delete[] *env;
			delete[] envp;

			delete[] argv[0];
			delete[] argv[1];
			delete[] argv;

			exit(EXIT_FAILURE);
		}
		catch (const std::exception &e)
		{
			std::cerr << "error in CGI execution: " << e.what() << std::endl;

			if (envp != NULL)
			{
				for (char **env = envp; *env != NULL; env++)
					delete[] *env;
				delete[] envp;
			}

			if (argv != NULL)
			{
				if (argv[0] != NULL)
					delete[] argv[0];
				if (argv[1] != NULL)
					delete[] argv[1];
				delete[] argv;
			}

			exit(EXIT_FAILURE);
		}
	}
	else // Parent process
	{
		closeFd(pipeIn[0]);	 // Close unused read end of pipeIn
		closeFd(pipeOut[1]); // Close unused write end of pipeOut

		setInFd(pipeIn[1]);
		setOutFd(pipeOut[0]);
		setPid(pid);
	}
}
