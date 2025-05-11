#pragma once
#include <unistd.h>
#include <string>

class HttpRequest;

class CGI
{
public:
	CGI();
	CGI(const CGI &other);
	CGI &operator=(const CGI &other);
	~CGI();

	void reset();

	int getInFd() const;
	void setInFd(int fd);

	int getOutFd() const;
	void setOutFd(int fd);

	pid_t getPid() const;
	void setPid(pid_t pid);

	int getCgiBodySentBytes() const;
	void setCgiBodySentBytes(int bytes);

	void start(const HttpRequest &request, const std::string &cgiPath, const std::string &scriptPath,
	          const std::string &localPort, const std::string &remoteHost, const std::string &uploadDir);

	char **createEnv(const HttpRequest &request, const std::string &scriptPath,
	               const std::string &localPort, const std::string &remoteHost,
				   const std::string &uploadDir) const;

private:
	int _inFd;
	int _outFd;
	pid_t _pid;
	size_t _cgiBodySentBytes;
};
