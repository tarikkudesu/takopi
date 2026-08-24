#include "Game.hpp"

Game::Game()
{
}
Game::Game(const Game &copy)
{
	*this = copy;
}
Game &Game::operator=(const Game &assign)
{
	if (this != &assign)
	{
	}
	return *this;
}
Game::~Game()
{
}

String Game::status()
{
	// this will return a game status string that will be sent to the GUI
}
void Game::setup()
{
	// setup the game, includes randomlly populating the world
}
void Game::mainLoop()
{
	// main loop with live updates
}
