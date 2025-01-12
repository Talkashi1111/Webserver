#include <vector>
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


#define PORT "9034" // Port we're listening on

void set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		perror("pollserver: fcntl F_GETFL error");
		if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		{
			perror("pollserver: fcntl O_NONBLOCK error");
		}
	}
	else
	{
		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			perror("pollserver: fcntl F_SETFL error");
		}
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

// Return a listening socket
int get_listener_socket()
{
	int listener; // Listening socket descriptor
	int yes = 1;  // For setsockopt() SO_REUSEADDR, below
	int rv;

	struct addrinfo hints, *ai, *p;

	// Get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
		throw std::runtime_error("pollserver: " + std::string(gai_strerror(rv)));

	for (p = ai; p != NULL; p = p->ai_next)
	{
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0)
			continue;
		// Avoid error of "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); // not need to check return value?
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
		{
			close(listener);
			continue;
		}
		break;
	}

	freeaddrinfo(ai); // All done with this

	// If we got here, it means we didn't get bound
	if (p == NULL)
		throw std::runtime_error("pollserver: failed to bind");

	// Set the socket to be non-blocking
	set_nonblocking(listener);

	// Listen
	if (listen(listener, 10) == -1)
	{
		close(listener);
		throw std::runtime_error("pollserver: failed to listen");
	}

	return listener;
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

void handle_new_connection(std::vector<struct pollfd> &pfds, int listener)
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
		set_nonblocking(newfd);
		add_to_pfds(pfds, newfd);
		std::cout << "pollserver: new connection from "
				  << inet_ntop(remoteaddr.ss_family,
							   get_in_addr((struct sockaddr *)&remoteaddr),
							   remoteIP, INET6_ADDRSTRLEN)
				  << " on socket " << newfd << std::endl;
	}
}

void handle_client_data(std::vector<struct pollfd> &pfds, int listener, int sender_fd)
{
	char buf[256];	// Buffer for client data

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
			if (dest_fd != listener && dest_fd != sender_fd)
			{
				if (send(dest_fd, buf, nbytes, 0) == -1)
				{
					perror("send");
				}
			}
		}
	}
}

void process_poll_events(std::vector<struct pollfd> &pfds, int listener)
{
	// Run through the existing connections looking for data to read
	for (size_t i = 0; i < pfds.size(); i++)
	{
		// Check if not ready to read by checking POLLIN bit
		if ((pfds[i].revents & POLLIN) == 0)
			continue;

		if (pfds[i].fd == listener)
		{
			// If listener is ready to read, handle new connection
			handle_new_connection(pfds, listener);
		}
		else
		{
			// If not the listener, we're just a regular client
			handle_client_data(pfds, listener, pfds[i].fd);
		} // END handle data from client
	} // END looping through file descriptors
}

int main()
{
	std::vector<struct pollfd> pfds;

	try
	{
		int listener = get_listener_socket(); // Listening socket descriptor
		add_to_pfds(pfds, listener);

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

			process_poll_events(pfds, listener);
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
