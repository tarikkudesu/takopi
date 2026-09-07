#include "Egg.hpp"

Egg::Egg() : __id(-1), __x(0), __y(0), __teamIndex(-1), __parentPlayerId(-1), __hatchTime(0), __state(EGG_INCUBATING)
{
}

Egg::Egg(int id, int parentPlayerId, int x, int y, int teamIndex, long hatchTime) : __id(id), __x(x), __y(y), __teamIndex(teamIndex), __parentPlayerId(parentPlayerId), __hatchTime(hatchTime), __state(EGG_INCUBATING)
{
}

Egg::Egg(const Egg &copy)
{
	*this = copy;
}

Egg &Egg::operator=(const Egg &assign)
{
	if (this != &assign)
	{
		__x = assign.__x;
		__y = assign.__y;
		__id = assign.__id;
		__state = assign.__state;
		__teamIndex = assign.__teamIndex;
		__hatchTime = assign.__hatchTime;
		__parentPlayerId = assign.__parentPlayerId;
	}
	return *this;
}

Egg::~Egg()
{
}

/*************************************************************************
 *                            ACCESSORS                                  *
 *************************************************************************/

int Egg::getId() const
{
	return __id;
}

int Egg::getX() const
{
	return __x;
}

int Egg::getParentPlayerId() const
{
	return __parentPlayerId;
}

int Egg::getY() const
{
	return __y;
}

int Egg::getTeamIndex() const
{
	return __teamIndex;
}

t_egg_state Egg::getState() const
{
	return __state;
}

bool Egg::isHatched() const
{
	return __state == EGG_HATCHED;
}

void Egg::setPosition(int x, int y)
{
	__x = x;
	__y = y;
}

/*************************************************************************
 *                            HATCHING                                   *
 *************************************************************************/

bool Egg::hatchIfReady(long currentTick)
{
	if (__state == EGG_INCUBATING && currentTick >= __hatchTime)
	{
		__state = EGG_HATCHED;
		return true;
	}
	return false;
}
