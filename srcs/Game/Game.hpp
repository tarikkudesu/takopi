#ifndef __GAME_HPP__
#define __GAME_HPP__

class Game
{
	private:
	public:
		Game();
		Game(const Game &copy);
		Game &operator=(const Game &assign);
		~Game();

        void mainLoop();
};


#endif
