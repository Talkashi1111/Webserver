#include "CGI.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>

CGI::CGI(const std::string &scriptPath,
		const std::map<std::string, std::string> &env,
		const std::string &postData)
	: _scriptPath(scriptPath), _env(env), _postData(postData)
{
	// Print the environment variables for debugging
	std::map<std::string, std::string>::iterator it;
	for (it = _env.begin(); it != _env.end(); ++it)
		std::cout << it->first << "=" << it->second << std::endl;
}

CGI::CGI(const CGI &src)
	: _scriptPath(src._scriptPath), _env(src._env), _postData(src._postData) {}

CGI &CGI::operator=(const CGI &src)
{
	if (this != &src) {
		_scriptPath = src._scriptPath;
		_env = src._env;
		_postData = src._postData;
	}
	return *this;
}

CGI::~CGI() {}

void CGI::prepareEnvironmentVariables()
{
	std::map<std::string, std::string>::iterator it;
	for (it = _env.begin(); it != _env.end(); ++it)
		setenv(it->first.c_str(), it->second.c_str(), 1);
}

std::string CGI::execute(std::string serverRoot)
{
	int stdin_fd[2];
	int stdout_fd[2];

	if (pipe(stdin_fd) == -1 || pipe(stdout_fd) == -1)
	{
		perror("pipe");
		throw std::runtime_error("500");
	}

	pid_t pid = fork();

	if (pid == -1)
	{
		perror("fork");
		close(stdin_fd[0]);
		close(stdin_fd[1]);
		close(stdout_fd[0]);
		close(stdout_fd[1]);
		throw std::runtime_error("500");
	}
	else if (pid == 0) // Child process
	{
		close(stdin_fd[1]); // Close unused write end of stdin pipe
		close(stdout_fd[0]); // Close unused read end of stdout pipe

		// Redirect stdin and stdout to the pipes
		dup2(stdin_fd[0], STDIN_FILENO);
		dup2(stdout_fd[1], STDOUT_FILENO);

		close(stdin_fd[0]);
		close(stdout_fd[1]);

		// Prepare environment variables
		prepareEnvironmentVariables();

		// Set the script path
		std::string scriptPath = serverRoot + _scriptPath;
		if (scriptPath.find(".php") != std::string::npos)
			execlp("/i=usr/bin/php-cgi", "php", scriptPath.c_str(), NULL);
		else if (scriptPath.find(".py") != std::string::npos)
			execlp("/usr/bin/python3", "python3", scriptPath.c_str(), NULL);
		else
			execl(scriptPath.c_str(), scriptPath.c_str(), NULL);
		perror("execl");
		exit(EXIT_FAILURE);
	}
	else // Parent process
	{
		close(stdin_fd[0]); // Close unused read end of stdin pipe
		close(stdout_fd[1]); // Close unused write end of stdout pipe

		// Write post data to the CGI script
		if (!_postData.empty())
		{
			write(stdin_fd[1], _postData.c_str(), _postData.size());
		}
		close(stdin_fd[1]);

		std::string response;
		char buffer[4096];
		ssize_t bytesRead;

		while ((bytesRead = read(stdout_fd[0], buffer, sizeof(buffer) - 1)) > 0)
		{
			buffer[bytesRead] = '\0';
			response += buffer;
		}

		close(stdout_fd[0]);
		waitpid(pid, NULL, 0); // Wait for the child process to finish

		return response;
	}
}
