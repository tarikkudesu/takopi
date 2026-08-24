#include "Game/Game.hpp"
#include "ServerManager/Core.hpp"

static void signalHandler(int signal)
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
	try
	{
		GameConfig config = Game::parseArgs(ac, av);
		Game game(config);
		game.start();
	}
	catch (const std::exception &e)
	{
		wsu::terr(e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
