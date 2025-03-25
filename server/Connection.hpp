#pragma once

#include <ctime>
#include <string>

class Server;
class Location;

class Connection
{
public:
	Connection(int fd, const std::string &port, const std::string &host);
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

private:
	int _fd;
	std::string _port;
	std::string _host;
	time_t _lastActivityTime;
	Server *_serverConfig;
	Location *_locationConfig;
};
