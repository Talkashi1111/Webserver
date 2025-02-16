#pragma once

#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdio>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <cerrno>

bool g_running = true;
const int kMaxEvents = 10;

// key: port number, value: a map of IP address and file descriptor
typedef std::map<std::string, int> ip_fd_map_t; // key: IP address, value: file descriptor
typedef std::map<std::string, ip_fd_map_t> bound_addrs_t; // key: port number, value: ip_fd_map_t

// TODO: delete
// {"8080": {"127.0.0.1": 3, "10.10.10.1": 4}, => {"8080": {"0.0.0.0": 3}
// "8081": {"172.30.30.23": 5},
// "8082": {"0.0.0.0": 6, "::": 7}}
