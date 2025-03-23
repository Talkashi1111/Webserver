#include <iostream>
#include <csignal>
#include <cstdio>
#include "Consts.hpp"
#include "Globals.hpp"
#include "WebServer.hpp"

bool g_running = true;

void sigintHandler(int)
{
	g_running = false;
}
//TODO: normalize functions naming, data mebers naming, and variables naming in all files.
int main(int argc, char *argv[])
{
	if (argc > 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}

	// Signal handling setup
	struct sigaction sa;
	sa.sa_handler = sigintHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		perror("sigaction");
		return 1;
	}

	try
	{
		std::string filename = (argc == 2) ? argv[1] : kDefaultConfig;
		WebServer ws(filename);
		if (DEBUG)
			ws.printSettings();
		// ws.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
