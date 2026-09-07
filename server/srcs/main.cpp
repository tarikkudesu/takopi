
#include "ServerManager/ServerManager.hpp"

void signalHandler(int signal)
{
	if (signal == SIGPIPE)
		mzu::warn("SIGPIPE");
	if (signal == SIGINT)
		Core::up = false;
}
void f()
{
    std::string cmd = "lsof -p " + std::to_string(getpid());
    system(cmd.c_str());
}

int main(int ac, char **av)
{
	atexit(f);
	if (ac == 1)
		exit(EXIT_FAILURE);
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, signalHandler);
	std::vector<String> args;
	for (int i = 1; i < ac; ++i)
		args.push_back(String(av[i]));
	bool success = false;
	{
		mzu::logs(args);
		ServerManager manager(*(args.end() - 1));
		success = manager.setUpZappy();
		mzu::close();
	}
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
