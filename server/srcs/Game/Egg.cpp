#include "Egg.hpp"

Egg::Egg() : __id(-1), __x(0), __y(0), __teamIndex(-1),
			 __hatchTime(0)
{
}

Egg::Egg(int id, int x, int y, int teamIndex, long hatchTime)
	: __id(id), __x(x), __y(y), __teamIndex(teamIndex),
	  __hatchTime(hatchTime)
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
		__id = assign.__id;
		__x = assign.__x;
		__y = assign.__y;
		__teamIndex = assign.__teamIndex;
		__hatchTime = assign.__hatchTime;
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

int Egg::getY() const
{
	return __y;
}

int Egg::getTeamIndex() const
{
	return __teamIndex;
}

void Egg::setPosition(int x, int y)
{
	__x = x;
	__y = y;
}

/*************************************************************************
 *                            HATCHING                                   *
 *************************************************************************/

bool Egg::tryHatch(long currentTick)
{
	if (currentTick >= __hatchTime)
	{
		return true;
	}
	return false;
}
