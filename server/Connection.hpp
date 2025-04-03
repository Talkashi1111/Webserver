#pragma once

#include <ctime>
#include <string>
#include "Consts.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class Server;
class Location;
class WebServer;

class Connection
{
public:
	Connection(int fd, const std::string &port, const std::string &host, int clientHeaderBufferSize, WebServer *ptr);
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

	time_t getLastActivityTime() const;
	void setLastActivityTime(time_t lastActivityTime);
	void updateActivityTime();

	Server *getServerConfig() const;
	void setServerConfig(Server *serverConfig);

	Location *getLocationConfig() const;
	void setLocationConfig(Location *locationConfig);

	const std::string &getResponse() const;
	void eraseResponse(int nbytes);

	bool isKeepAlive() const;

	RequestState handleClientRecv(const std::string &raw);
	void reset();

private:
	int _fd;
	WebServer *_webserver;
	std::string _port;
	std::string _host;
	time_t _lastActivityTime;
	Server *_serverConfig;
	Location *_locationConfig;
	HttpRequest _request;
	HttpResponse _response;
	int _clientHeaderBufferSize;
	bool _keepAlive;
};
