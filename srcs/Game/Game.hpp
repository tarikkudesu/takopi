#ifndef __GAME_HPP__
#define __GAME_HPP__

# include "../utilities/BasicString.hpp"

class Game
{
	private:
	public:
		Game();
		Game(const Game &copy);
		Game &operator=(const Game &assign);
		~Game();

		String status();
		void setup();
		void mainLoop();
};


#endif
