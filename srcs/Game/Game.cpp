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
