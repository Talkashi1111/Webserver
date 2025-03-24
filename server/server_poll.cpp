#include "server_poll.hpp"

void setNonblocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		int err = errno;
		// TODO: print error? or should throw exception? or just close the socket?
		std::cerr << "fcntl O_NONBLOCK error (" << fd << "): " << strerror(err) << std::endl;
	}
}

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) // IPv4
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr); // IPv6
}

std::string getStraddr(struct addrinfo *p)
{
	char ipstr[INET6_ADDRSTRLEN];
	void *addr;

	// Get the pointer to the address itself,
	// different fields in IPv4 and IPv6:
	if (p->ai_family == AF_INET) // IPv4
	{
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		addr = &(ipv4->sin_addr);
	}
	else // IPv6
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
		addr = &(ipv6->sin6_addr);
	}

	// Convert the IP to a string and print it:
	inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
	return std::string(ipstr);
}

void printAddrinfo(const char *host, const char *port, struct addrinfo *ai)
{
	struct addrinfo *p;

	std::cout << "addrinfo for " << host << ":" << port << std::endl;
	for (p = ai; p != NULL; p = p->ai_next)
	{
		std::string ipver = "IPv4";
		if (p->ai_family == AF_INET6)
			ipver = "IPv6";
		std::string straddr = getStraddr(p);
		std::cout << "\t" << ipver << ": " << straddr << std::endl;
	}
}

void close_sockets(ip_fd_map_t &ip_fd_map)
{
	for (ip_fd_map_t::iterator it = ip_fd_map.begin(); it != ip_fd_map.end(); ++it)
	{
		if (close(it->second) == -1)
		{
			int err = errno;
			std::cerr << "close (" << it->second << "): " << strerror(err) << std::endl;
		}
	}
}

void closeAllSockets(bound_addrs_t &bound_addrs4, bound_addrs_t &bound_addrs6)
{
	for (bound_addrs_t::iterator bp = bound_addrs4.begin(); bp != bound_addrs4.end(); ++bp)
	{
		close_sockets(bp->second);
	}
	for (bound_addrs_t::iterator bp = bound_addrs6.begin(); bp != bound_addrs6.end(); ++bp)
	{
		close_sockets(bp->second);
	}
}

bool check_binding(const std::string& all_interfaces, const std::string& port,
					const std::string& straddr, bound_addrs_t& bound_addrs)
{
	bound_addrs_t::iterator bp = bound_addrs.find(port);
	if (bp != bound_addrs.end()) // if port in map
	{
		if (bp->second.find(all_interfaces) != bp->second.end())
		{
			// Already bound to all addresses on this port
			return true;
		}
		if (bp->second.find(straddr) != bp->second.end())
		{
			// Already bound to this address and port
			return true;
		}
		if (straddr == all_interfaces)
		{
			close_sockets(bp->second);
			bp->second.clear();
		}
	}
	return false;
}

std::set<int> get_fd_set(bound_addrs_t& bound_addrs4, bound_addrs_t& bound_addrs6)
{
	std::set<int> fd_set;
	for (bound_addrs_t::iterator bp = bound_addrs4.begin(); bp != bound_addrs4.end(); ++bp)
	{
		for (ip_fd_map_t::iterator ip = bp->second.begin(); ip != bp->second.end(); ++ip)
		{
			fd_set.insert(ip->second);
		}
	}
	for (bound_addrs_t::iterator bp = bound_addrs6.begin(); bp != bound_addrs6.end(); ++bp)
	{
		for (ip_fd_map_t::iterator ip = bp->second.begin(); ip != bp->second.end(); ++ip)
		{
			fd_set.insert(ip->second);
		}
	}
	return fd_set;
}

std::set<int> get_listener_socket(int argc, char *argv[])
{
	bound_addrs_t bound_addrs4;
	bound_addrs_t bound_addrs6;
	int yes = 1; // For setsockopt() SO_REUSEADDR, below
	int rv;
	std::string strerr;

	struct addrinfo hints, *ai, *p;

	// Get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	for (int i = 1; i < argc; i += 2)
	{
		// TODO: replace argv and argc by config file data
		// argv[i] is host, argv[i + 1] is port
		if ((rv = getaddrinfo(argv[i], argv[i + 1], &hints, &ai)) != 0)
		{
			strerr = "getaddrinfo(" + std::string(argv[i]) + ":" + argv[i + 1] + "): " + std::string(gai_strerror(rv));
			throw std::runtime_error(strerr);
		}

		// TODO: should be debug print
		printAddrinfo(argv[i], argv[i + 1], ai);

		for (p = ai; p != NULL; p = p->ai_next)
		{
			std::string straddr = getStraddr(p);
			if (p->ai_family == AF_INET) // IPv4
			{
				if (check_binding("0.0.0.0", argv[i + 1], straddr, bound_addrs4))
					continue;
			}
			if (p->ai_family == AF_INET6) // IPv6
			{
				if (check_binding("::", argv[i + 1], straddr, bound_addrs6))
					continue;
			}

			int listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (listener < 0)
			{
				int err = errno;
				strerr = "socket(" + straddr + ":" + argv[i + 1] + "): " + strerror(err);
				closeAllSockets(bound_addrs4, bound_addrs6);
				freeaddrinfo(ai);
				throw std::runtime_error(strerr);
			}
			// Avoid error of "address already in use" error message
			setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); // don't care about return value, do the best effort
			if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
			{
				int err = errno;
				strerr = "bind(" + straddr + ":" + argv[i + 1] + "): " + strerror(err);
				close(listener);
				closeAllSockets(bound_addrs4, bound_addrs6);
				freeaddrinfo(ai);
				throw std::runtime_error(strerr);
			}

			// Set the socket to be non-blocking
			setNonblocking(listener);

			// Listen
			if (listen(listener, 10) == -1)
			{
				int err = errno;
				strerr = "listen(" + straddr + ":" + argv[i + 1] + "): " + strerror(err);
				close(listener);
				closeAllSockets(bound_addrs4, bound_addrs6);
				freeaddrinfo(ai);
				throw std::runtime_error(strerr);
			}

			if (p->ai_family == AF_INET) // IPv4
				bound_addrs4[argv[i + 1]][getStraddr(p)] = listener;
			else // IPv6
				bound_addrs6[argv[i + 1]][getStraddr(p)] = listener;
		}
		freeaddrinfo(ai); // All done with this structure
	}
	return get_fd_set(bound_addrs4, bound_addrs6);
}

// Add a new file descriptor to the set
void add_to_pfds(std::vector<struct pollfd> &pfds, std::set<int> &newfds)
{
	std::set<int>::iterator it;
	for (it = newfds.begin(); it != newfds.end(); ++it)
	{
		struct pollfd pfd;
		pfd.fd = *it;
		// TODO: handle POLLOUT
		pfd.events = POLLIN; // Check ready-to-read
		pfds.push_back(pfd);
	}
}

// Add a new file descriptor to the set
void add_to_pfds(std::vector<struct pollfd> &pfds, int newfd)
{
	struct pollfd pfd;
	pfd.fd = newfd;
	// TODO: handle POLLOUT
	pfd.events = POLLIN; // Check ready-to-read
	pfds.push_back(pfd);
}

// Remove an index from the set
void del_from_pfds(std::vector<struct pollfd> &pfds, int index)
{
	pfds.erase(pfds.begin() + index);
}

void handleNewConnection(std::vector<struct pollfd> &pfds, int listener)
{
	socklen_t addrlen;
	struct sockaddr_storage remoteaddr; // Client address
	int newfd;							// Newly accept()ed socket descriptor
	char remoteIP[INET6_ADDRSTRLEN];

	// If listener is ready to read, handle new connection
	addrlen = sizeof remoteaddr;
	newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
	if (newfd == -1)
	{
		perror("accept");
	}
	else
	{
		// Set the new socket to non-blocking mode
		setNonblocking(newfd);
		add_to_pfds(pfds, newfd);
		std::cout << "pollserver: new connection from "
				  << inet_ntop(remoteaddr.ss_family,
							   get_in_addr((struct sockaddr *)&remoteaddr),
							   remoteIP, INET6_ADDRSTRLEN)
				  << " on socket " << newfd << std::endl;
	}
}

void handleClientData(std::vector<struct pollfd> &pfds, std::set<int> listeners, int sender_fd)
{
	char buf[256]; // Buffer for client data

	int nbytes = recv(sender_fd, buf, sizeof buf, 0);
	if (nbytes <= 0)
	{
		// Got error or connection closed by client
		if (nbytes == 0)
		{
			// Connection closed
			std::cout << "pollserver: socket " << sender_fd << " hung up" << std::endl;
		}
		else
		{
			perror("recv");
		}
		close(sender_fd);
		del_from_pfds(pfds, sender_fd);
	}
	else
	{
		// We got some good data from a client
		for (size_t j = 0; j < pfds.size(); j++)
		{
			// Send to everyone!
			int dest_fd = pfds[j].fd;

			// Except the listener and ourselves
			if (listeners.find(dest_fd) != listeners.end() || dest_fd == sender_fd)
				continue;

			if (send(dest_fd, buf, nbytes, 0) == -1)
			{
				perror("send");
			}
		}
	}
}

void processPollEvents(std::vector<struct pollfd> &pfds, std::set<int> listeners)
{
	// Run through the existing connections looking for data to read
	for (size_t i = 0; i < pfds.size(); i++)
	{
		// Check if not ready to read by checking POLLIN bit
		if ((pfds[i].revents & POLLIN) == 0)
			continue;

		if (listeners.find(pfds[i].fd) != listeners.end())
		{
			// If listener is ready to read, handle new connection
			handleNewConnection(pfds, pfds[i].fd);
		}
		else
		{
			// If not the listener, we're just a regular client
			handleClientData(pfds, listeners, pfds[i].fd);
		} // END handle data from client
	} // END looping through file descriptors
}

int main(int argc, char *argv[])
{
	std::vector<struct pollfd> pfds;
	std::set<int> listeners;

	try
	{
		listeners = get_listener_socket(argc, argv);
		add_to_pfds(pfds, listeners);

		// Main loop
		for (;;)
		{
			int poll_count = poll(pfds.data(), pfds.size(), -1); // infinity timeout
			if (poll_count == -1)
			{
				perror("poll");
				continue; // TODO: how to handle EINTR? in case of SIGCHLD after fork
						  // when child process is terminated and parent registered signal handler on SIGCHLD
			}

			processPollEvents(pfds, listeners);
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
