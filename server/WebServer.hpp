#pragma once

#include <map>
#include <set>
#include <sys/epoll.h>
#include "Consts.hpp"
#include "ServerKey.hpp"
#include "Server.hpp"

// key: port number, value: a map of IP address and file descriptor
typedef std::map<std::string, int> ip_fd_map_t;			  // key: IP address, value: file descriptor
typedef std::map<std::string, ip_fd_map_t> bound_addrs_t; // key: port number, value: ip_fd_map_t

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
	void close_all_sockets(bound_addrs_t &bound_addrs4, bound_addrs_t &bound_addrs6);
	bool check_binding(const std::string &all_interfaces, const std::string &port,
					   const std::string &straddr, bound_addrs_t &bound_addrs);
	std::set<int> get_fd_set(bound_addrs_t &bound_addrs4, bound_addrs_t &bound_addrs6);
	// void setup_listener_sockets();
	void handle_new_connection(int epfd, int listener);
	void handle_client_data(int epfd, int sender_fd);
	void set_nonblocking(int fd);
	void cleanup(int epfd, std::set<int> &listeners);
	void printSettings() const;

private:
std::string file_name;
	int epfd;
	std::set<int> listeners;
	struct epoll_event evlist[kMaxEvents];
	std::map<ServerKey, Server *> _servers;

	void parseConfig();
	int initEpoll();
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
	void handleServerDirective(const std::vector<std::string> &words, Server *curr_server);
	void handleLocationDirective(const std::vector<std::string> &words, Location *curr_location);
	void inheritServerDirectives(Server* curr_server);
	void addServer(Server *server);
	void cleanupAllocatedMemory();
};
