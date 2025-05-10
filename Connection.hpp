#pragma once

#include <ctime>
#include <string>
#include "Consts.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "CGI.hpp"

class Server;
class Location;
class WebServer;

class Connection
{
public:
	Connection(int fd,
			   const std::string &port, const std::string &host,
			   const std::string &remotePort, const std::string &remoteHost,
			   WebServer *ptr);
	Connection(const Connection &connection);
	Connection &operator=(const Connection &connection);
	~Connection();

	// Setters / Getters
	int getFd() const;
	void setFd(int fd);

	const std::string &getPort() const;
	void setPort(const std::string &port);
	const std::string &getHost() const;
	void setHost(const std::string &host);

	const std::string &getRemotePort() const;
	void setRemotePort(const std::string &remotePort);
	const std::string &getRemoteHost() const;
	void setRemoteHost(const std::string &remoteHost);

	time_t getLastActivityTime() const;
	void setLastActivityTime(time_t lastActivityTime);
	void updateActivityTime();

	Server *getServerConfig() const;
	void setServerConfig(Server *serverConfig);

	Location *getLocationConfig() const;
	void setLocationConfig(Location *locationConfig);

	const std::string &getResponse() const;
	void eraseResponse(int nbytes);

	pid_t getCgiPid() const;
	void setCgiPid(pid_t pid);

	int getCgiInFd() const;
	void setCgiInFd(int fd);

	int getCgiOutFd() const;
	void setCgiOutFd(int fd);

	bool isKeepAlive() const;
	bool isAllowdMethod(const std::string &method, const std::map<std::string, bool> methods) const;
	std::string generateAutoIndex(const std::string &path) const;
	RequestState handleClientRecv(const std::string &raw);
	RequestState handleCgiRecv(int fd);
	RequestState finalizeCgiRecv(int fd);
	RequestState handleCgiSend(int fd);
	void reset();

private:
	int _fd;
	WebServer *_webserver;
	std::string _port;
	std::string _host;
	std::string _remotePort;
	std::string _remoteHost;
	time_t _lastActivityTime;
	Server *_serverConfig;
	Location *_locationConfig;
	CGI _cgi;
	HttpRequest _request;
	HttpResponse _response;
	bool _keepAlive;

	void setServerAndLocation();
	std::string resolvePath(const std::string &root, const std::string &path) const;
	void generateReturnDirectiveResponse(const std::string &status, const std::string &redirectPath);
	void generateResponse();
	std::string getCgiPath(const std::string &path) const;
	void setContentType(const std::string &path, std::ostringstream &oss);
	bool processCgiHeaders(const std::string &cgiData, std::string &statusCode,
						   std::map<std::string, std::string> &cgiHeaders,
						   std::string &body);
	std::string buildHttpResponse(const std::string &statusCode,
								  const std::map<std::string, std::string> &headers,
								  const std::string &body);
};
