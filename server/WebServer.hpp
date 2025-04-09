#pragma once

#include <map>
#include <set>
#include <sys/epoll.h>
#include "Consts.hpp"
#include "ServerKey.hpp"

class Server;
class Location;
class Connection;

const int kMaxEvents = 10;

enum ParseState
{
	GLOBAL,
	SERVER,
	LOCATION
};

class WebServer
{
public:
	WebServer(const std::string &filename);
	WebServer(const WebServer &ws);
	WebServer &operator=(const WebServer &ws);
	~WebServer();

	void run();
	void printSettings() const;
	void closeExpiredConnections();

	// Setters / Getters
	void setClientTimeout(int timeout);
	int getClientTimeout() const;
	bool isClientTimeoutSet() const;
	int getEpollFD() const;

	// Convert a size string (e.g., "1k", "2m") to bytes.
	// possible suffixes: k, K, m, M or none (bytes)
	void setClientHeaderBufferSize(const std::string &size);
	int getClientHeaderBufferSize() const;
	bool isClientHeaderBufferSizeSet() const;

	// possible suffixes: k, K, m, M or none (bytes)
	void setClientMaxBodySize(const std::string &size);
	size_t getClientMaxBodySize() const;
	bool isClientMaxBodySizeSet() const;

	const std::map<ServerKey, Server*>& getServers() const;

private:
	std::string _fileName;
	int _epfd;
	std::map<int, std::pair<std::string, std::string> > _listeners; // key: file descriptor, value: pair of local host and port
	int _clientTimeout;											   // in seconds; Default: 75
	bool _clientTimeoutSet;
	int _clientHeaderBufferSize; 								   // in bytes; Default: 1k
	bool _clientHeaderBufferSizeSet;
	size_t _clientMaxBodySize;						 				   // in bytes; Default: 1m
	bool _clientMaxBodySizeSet;
	struct epoll_event _evlist[kMaxEvents];
	std::map<ServerKey, Server *> _servers;
	std::map<int, Connection *> _connections; // key: file descriptor, value: Connection object

	void parseConfig();
	void initEpoll();
	std::string readUntilDelimiter(std::istream &file, const std::string &delimiters);
	void handleOpenBracket(const std::string &content_block, Server *&curr_server, Location *&curr_location, ParseState &state);
	void handleDirective(const std::string &content_block, Server *curr_server, Location *curr_location, ParseState &state);
	void handleCloseBracket(const std::string &content_block, Server *&curr_server, Location *&curr_location, ParseState &state);
	void resolveAndAddListen(const std::string &listen, Server *server);
	void validateSizeFormat(const std::string &size);
	void handle_cgi_bin_directive(const std::vector<std::string> &words, Server *curr_server);
	void validateUrl(const std::string &url) const;
	void validateReturnDirective(const std::vector<std::string> &words);
	void validateRootDirective(const std::vector<std::string> &words);
	void handleGlobalDirective(const std::vector<std::string> &words);
	void handleServerDirective(const std::vector<std::string> &words, Server *curr_server);
	void handleLocationDirective(const std::vector<std::string> &words, Location *curr_location);
	void inheritServerDirectives(Server *curr_server);
	void addServer(Server *server);
	void processPollEvents(int ready);
	bool updateEpollEvents(int fd, uint32_t events);
	void closeListenerSockets();
	void cleanupServers();
	void cleanupConnections();
	void handleConnectionClose(int fd);
	void setupListenerSockets();
	void handleNewConnection(int listener);
	void handleClientRecv(int fd);
	void handleClientSend(int fd);
	bool setNonblocking(int fd);
};
