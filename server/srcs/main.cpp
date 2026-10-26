
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
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, signalHandler);

	t_svec args;
	for (int i = 1; i < ac; ++i)
		args.push_back(String(av[i]));

	bool success = false;
	{
		t_args overrides = mzu::argparse(args);
		ServerManager manager(*(args.end() - 1));
		success = manager.runzappy(overrides);
		mzu::close();
	}
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
