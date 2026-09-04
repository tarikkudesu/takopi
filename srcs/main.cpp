
#include "ServerManager/ServerManager.hpp"

void signalHandler(int signal)
{
	if (signal == SIGPIPE)
		mzu::warn("SIGPIPE");
	if (signal == SIGINT)
		Core::up = false;
}

int main(int ac, char **av)
{
	if (ac == 1)
		exit(EXIT_FAILURE);
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, signalHandler);
	std::vector<String> args;
	for (int i = 1; i < ac; ++i)
		args.push_back(String(av[i]));
	{
		mzu::logs(args);
		ServerManager manager(*(args.end() - 1));
		manager.setUpZappy();
	}
	exit(EXIT_FAILURE);
}
