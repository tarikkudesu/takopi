#include "Player.hpp"

Player::Player()
	: __id(-1), __x(0), __y(0), __level(1), __state(PLAYER_DEAD), __direction(NORTH), __teamIndex(-1), __nextFoodTick(0)
{
	for (int i = 0; i < RESOURCE_COUNT; i++)
		__inventory[i] = 0;
}

Player::Player(int id, int x, int y, int teamIndex, long spawnTick)
	: __id(id), __x(x), __y(y), __level(1), __state(PLAYER_ALIVE), __direction(NORTH), __teamIndex(teamIndex), __nextFoodTick(spawnTick + FOOD_LIFE_UNITS)
{
	for (int i = 0; i < RESOURCE_COUNT; i++)
		__inventory[i] = 0;
	__inventory[NOURRITURE] = INITIAL_FOOD;
}

Player::Player(const Player &copy)
{
	*this = copy;
}

Player &Player::operator=(const Player &assign)
{
	if (this != &assign)
	{
		__id = assign.__id;
		__x = assign.__x;
		__y = assign.__y;
		__level = assign.__level;
		__state = assign.__state;
		__teamIndex = assign.__teamIndex;
		__direction = assign.__direction;
		__nextFoodTick = assign.__nextFoodTick;
		for (int i = 0; i < RESOURCE_COUNT; i++)
			__inventory[i] = assign.__inventory[i];
	}
	return *this;
}

Player::~Player()
{
}

/*************************************************************************
 *                            ACCESSORS                                  *
 *************************************************************************/

int Player::getId() const
{
	return __id;
}

int Player::getX() const
{
	return __x;
}

int Player::getY() const
{
	return __y;
}

int Player::getLevel() const
{
	return __level;
}

int Player::getTeamIndex() const
{
	return __teamIndex;
}

Direction Player::getDirection() const
{
	return __direction;
}

int Player::getInventory(Resource type) const
{
	if (type < 0 || type >= RESOURCE_COUNT)
		return 0;
	return __inventory[type];
}

long Player::getNextFoodTick() const
{
	return __nextFoodTick;
}

/*************************************************************************
 *                            MUTATORS                                   *
 *************************************************************************/

void Player::setPosition(int x, int y)
{
	__x = x;
	__y = y;
}

void Player::setLevel(int level)
{
	__level = level;
}

void Player::setState(PlayerState state)
{
	__state = state;
}

void Player::setDirection(Direction dir)
{
	__direction = dir;
}

void Player::setNextFoodTick(long tick)
{
	__nextFoodTick = tick;
}

/*************************************************************************
 *                            MOVEMENT                                   *
 *************************************************************************/

void Player::moveInDirection(Direction dir, int mapW, int mapH)
{
	switch (dir)
	{
		case NORTH:
			__y = ((__y - 1) % mapH + mapH) % mapH;
			break;
		case SOUTH:
			__y = (__y + 1) % mapH;
			break;
		case EAST:
			__x = (__x + 1) % mapW;
			break;
		case WEST:
			__x = ((__x - 1) % mapW + mapW) % mapW;
			break;
	}
}

void Player::moveForward(int mapW, int mapH)
{
	moveInDirection(__direction, mapW, mapH);
}

void Player::turnRight()
{
	__direction = static_cast<Direction>((__direction + 1) % 4);
}

void Player::turnLeft()
{
	__direction = static_cast<Direction>((__direction + 3) % 4);
}

/*************************************************************************
 *                           INVENTORY                                   *
 *************************************************************************/

void Player::addToInventory(Resource type, int count)
{
	if (type >= 0 && type < RESOURCE_COUNT)
		__inventory[type] += count;
}

bool Player::removeFromInventory(Resource type)
{
	if (type >= 0 && type < RESOURCE_COUNT && __inventory[type] > 0)
	{
		__inventory[type]--;
		return true;
	}
	return false;
}

String Player::inventoryString() const
{
	String result = "{";
	result += "nourriture " + 	mzu::intToString(__inventory[NOURRITURE]);
	result += ", linemate " + 	mzu::intToString(__inventory[LINEMATE]);
	result += ", deraumere " + 	mzu::intToString(__inventory[DERAUMERE]);
	result += ", sibur " + 		mzu::intToString(__inventory[SIBUR]);
	result += ", mendiane " + 	mzu::intToString(__inventory[MENDIANE]);
	result += ", phiras " + 	mzu::intToString(__inventory[PHIRAS]);
	result += ", thystame " + 	mzu::intToString(__inventory[THYSTAME]);
	result += "}";
	return result;
}

bool Player::isAlive() const
{
	return __state == PLAYER_ALIVE;
}
