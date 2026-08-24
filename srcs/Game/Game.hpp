#ifndef __GAME_HPP__
#define __GAME_HPP__

#include "../utilities/BasicString.hpp"

class Server;

struct GameConfig
{
	int								port;
	bool							portSet;
	int								width;
	int								height;
	t_svec							teams;
	String							configPath;

	GameConfig();
};

class Game
{
	private:
		GameConfig						__config;

		Game(const Game &copy);
		Game	&operator=(const Game &assign);

	public:
		static GameConfig				parseArgs(int ac, char **av);
		void							start();

		Game(const GameConfig &config);
		~Game();
};

#endif
