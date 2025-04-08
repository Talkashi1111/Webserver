#include "CGI.hpp"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <sys/stat.h>
#include <cstring>
#include <unistd.h>
#include <stdexcept>
#include <sys/wait.h>

CGI::CGI(const std::string &cgiDirectory) : _cgiDirectory(cgiDirectory)
{
	if (cgiDirectory.empty())
		throw std::runtime_error("CGI directory is empty");
}

CGI::CGI(const CGI &src)
{
	*this = src;
}

CGI &CGI::operator=(const CGI &src)
{
	if (this != &src)
	{
		_cgiDirectory = src._cgiDirectory;
		_env = src._env;
	}
	return *this;
}

CGI::~CGI() {}

std::string CGI::execute(const std::string &scriptPath, const HttpRequest &request)
{
	HttpResponse response;
	std::string method = request.getMethod();
	std::string queryString = request.getQuery();
	std::string body = request.getBody();
	std::map<std::string, std::string> headers = request.getHeaders();
	std::string fullScriptPath = scriptPath;

	setEnvironmentVariables(request, scriptPath);

	std::cout << "Executing CGI script: " << fullScriptPath << std::endl;
	struct stat st;
	if (stat(fullScriptPath.c_str(), &st) == -1)
	{
		perror("stat");
		throw std::runtime_error("404");
	}
	if (!S_ISREG(st.st_mode))
	{
		std::cerr << "Not a regular file: " << fullScriptPath << std::endl;
		throw std::runtime_error("404");
	}
	if (access(fullScriptPath.c_str(), X_OK) == -1)
	{
		perror("access");
		throw std::runtime_error("403");
	}

	int pipeStdOut[2];
	int pipeStdIn[2];

	if (pipe(pipeStdOut) == -1 || pipe(pipeStdIn) == -1)
	{
		perror("pipe");
		throw std::runtime_error("500");
	}

	pid_t pid = fork();

	if (pid == -1)
	{
		perror("fork");
		throw std::runtime_error("500");
	}
	else if (pid == 0) // Child process
	{
		dup2(pipeStdOut[1], STDOUT_FILENO);
		close(pipeStdOut[0]); // Close read end of stdout pipe
		close(pipeStdOut[1]); // Close write end of stdout pipe

		if (method == "POST" || method == "DELETE")
			dup2(pipeStdIn[0], STDIN_FILENO);
		close(pipeStdIn[0]); // Close read end of stdin pipe
		close(pipeStdIn[1]); // Close write end of stdin pipe

		// Execute the CGI script
		std::vector<char*> argv;
		argv.push_back(const_cast<char*>(fullScriptPath.c_str()));
		argv.push_back(NULL);

		execve(fullScriptPath.c_str(), argv.data(), _env.data());
		perror("execve");
		throw std::runtime_error("500");
	}
	else // Parent process
	{
		close(pipeStdOut[1]); // Close write end of stdout pipe
		close(pipeStdIn[0]);  // Close read end of stdin pipe

		if (method == "POST" || method == "DELETE")
			write(pipeStdIn[1], body.c_str(), body.length());
		close(pipeStdIn[1]); // Close write end of stdin pipe

		std::string responseBody;
		char buffer[4096];
		ssize_t bytesRead;
		while ((bytesRead = read(pipeStdOut[0], buffer, sizeof(buffer))) > 0)
			responseBody.append(buffer, bytesRead);
		close(pipeStdOut[0]); // Close read end of stdout pipe

		int status;
		waitpid(pid, &status, 0); // Wait for child process to finish

		return responseBody;
	}
}

void CGI::setEnvironmentVariables(const HttpRequest &request, const std::string &scriptPath)
{
	std::vector<std::string> env;

	env.push_back("SERVER_SOFTWARE=webserver/1.0");
	env.push_back("SERVER_NAME=" + request.getHostName());
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("REQUEST_METHOD=" + request.getMethod());
	env.push_back("SCRIPT_NAME=" + scriptPath);
	// env.push_back("PATH_INFO=" + request.getPath());
	env.push_back("PATH_TRANSLATED=");
	if (request.getMethod() == "GET" && !request.getQuery().empty())
		env.push_back("QUERY_STRING=" + request.getQuery());
	if (request.getMethod() == "POST" || request.getMethod() == "DELETE")
	{
		std::stringstream ss;
		ss << request.getBody().length();
		env.push_back("CONTENT_LENGTH=" + ss.str());
		if (request.getHeaders().find("content-type") != request.getHeaders().end())
			env.push_back("CONTENT_TYPE=" + request.getHeaders().find("content-type")->second);
		else
			env.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
	}

	std::map<std::string, std::string>::const_iterator it1;
	for (it1 = request.getHeaders().begin(); it1 != request.getHeaders().end(); ++it1)
	{
		std::string headerName = it1->first;
		std::string headerValue = it1->second;
		if (headerName.find("HTTP_") == std::string::npos)//TODO: verify if this is correct
			headerName = "HTTP_" + headerName;
		env.push_back(headerName + "=" + headerValue);
	}

	_env.clear();
	std::vector<std::string>::iterator it2;
	for (it2 = env.begin(); it2 != env.end(); ++it2)
		_env.push_back(const_cast<char*>(it2->c_str()));
	_env.push_back(NULL);
}
