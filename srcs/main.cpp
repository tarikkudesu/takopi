
#include "ServerManager/ServerManager.hpp"

void signalHandler(int signal)
{
	if (signal == SIGPIPE)
		wsu::warn("SIGPIPE");
	if (signal == SIGINT)
		Core::up = false;
}

int main(int ac, char **av)
{
	signal(SIGPIPE, signalHandler);
	signal(SIGINT, signalHandler);
	std::vector<String> args;
	for (int i = 1; i < ac; ++i)
		args.push_back(String(av[i]));
	{
		wsu::logs(args);
		ServerManager webserv(*(args.end() - 1));
		webserv.setUpWebserv();
	}
	exit(EXIT_FAILURE);
}
