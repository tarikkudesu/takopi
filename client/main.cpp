#include "Game/Client.hpp"

static void signalHandler(int signal)
{
	if (signal == SIGINT)
		Client::stop();
}

static void printUsage(const char *prog)
{
	std::cerr << "USAGE: " << prog << " -n <team> -p <port> [-h <hostname>]" << std::endl;
}

int main(int ac, char **av)
{
	String team;
	String host = DEFAULT_HOST;
	int port = -1;

	for (int i = 1; i < ac; i++)
	{
		String arg = av[i];
		if (arg == "-n" && i + 1 < ac)
			team = av[++i];
		else if (arg == "-p" && i + 1 < ac)
			port = static_cast<int>(mzu::stringToInt(av[++i]));
		else if (arg == "-h" && i + 1 < ac)
			host = av[++i];
		else
		{
			printUsage(av[0]);
			return EXIT_FAILURE;
		}
	}
	if (team.empty() || port < 1 || port > 65535)
	{
		printUsage(av[0]);
		return EXIT_FAILURE;
	}

	signal(SIGINT, signalHandler);
	signal(SIGPIPE, SIG_IGN);

	try
	{
		Client client(team, host, port);
		client.run();
	}
	catch (std::exception &e)
	{
		mzu::terr(e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
