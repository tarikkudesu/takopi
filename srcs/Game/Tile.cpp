#include "Tile.hpp"

Tile::Tile() : __x(0), __y(0)
{
	for (int i = 0; i < RESOURCE_COUNT; i++)
		__resources[i] = 0;
}

Tile::Tile(int x, int y) : __x(x), __y(y)
{
	for (int i = 0; i < RESOURCE_COUNT; i++)
		__resources[i] = 0;
}

Tile::Tile(const Tile &copy)
{
	*this = copy;
}

Tile &Tile::operator=(const Tile &assign)
{
	if (this != &assign)
	{
		__x = assign.__x;
		__y = assign.__y;
		for (int i = 0; i < RESOURCE_COUNT; i++)
			__resources[i] = assign.__resources[i];
		__playerIds = assign.__playerIds;
	}
	return *this;
}

Tile::~Tile()
{
}

/*************************************************************************
 *                            ACCESSORS                                  *
 *************************************************************************/

int Tile::getX() const
{
	return __x;
}

int Tile::getY() const
{
	return __y;
}

int Tile::getResource(e_resource type) const
{
	if (type < 0 || type >= RESOURCE_COUNT)
		return 0;
	return __resources[type];
}

const std::vector<int> &Tile::getPlayerIds() const
{
	return __playerIds;
}

/*************************************************************************
 *                            MUTATORS                                   *
 *************************************************************************/

void Tile::addResource(e_resource type, int count)
{
	if (type >= 0 && type < RESOURCE_COUNT)
		__resources[type] += count;
}

void Tile::removeResource(e_resource type)
{
	if (type >= 0 && type < RESOURCE_COUNT && __resources[type] > 0)
		__resources[type]--;
}

bool Tile::hasResource(e_resource type) const
{
	if (type < 0 || type >= RESOURCE_COUNT)
		return false;
	return __resources[type] > 0;
}

void Tile::addPlayer(int playerId)
{
	__playerIds.push_back(playerId);
}

void Tile::removePlayer(int playerId)
{
	for (std::vector<int>::iterator it = __playerIds.begin(); it != __playerIds.end(); ++it)
	{
		if (*it == playerId)
		{
			__playerIds.erase(it);
			return;
		}
	}
}

/*************************************************************************
 *                         CONTENT STRING                                *
 *************************************************************************/

String Tile::contentString() const
{
	static const char *names[] = {
		"nourriture", "linemate", "deraumere", "sibur",
		"mendiane", "phiras", "thystame"
	};
	String result;
	for (size_t i = 0; i < __playerIds.size(); i++)
	{
		if (!result.empty())
			result += " ";
		result += "player";
	}
	for (int r = 0; r < RESOURCE_COUNT; r++)
	{
		for (int c = 0; c < __resources[r]; c++)
		{
			if (!result.empty())
				result += " ";
			result += names[r];
		}
	}
	return result;
}
