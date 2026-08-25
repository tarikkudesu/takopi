#include "Egg.hpp"

Egg::Egg() : __id(-1), __x(0), __y(0), __teamIndex(-1),
			 __layerId(-1), __hatchTime(0), __hatched(false)
{
}

Egg::Egg(int id, int x, int y, int teamIndex, int layerId, long hatchTime)
	: __id(id), __x(x), __y(y), __teamIndex(teamIndex),
	  __layerId(layerId), __hatchTime(hatchTime), __hatched(false)
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
		__layerId = assign.__layerId;
		__hatchTime = assign.__hatchTime;
		__hatched = assign.__hatched;
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

int Egg::getLayerId() const
{
	return __layerId;
}

long Egg::getHatchTime() const
{
	return __hatchTime;
}

bool Egg::isHatched() const
{
	return __hatched;
}

/*************************************************************************
 *                            HATCHING                                   *
 *************************************************************************/

bool Egg::tryHatch(long currentTick)
{
	if (!__hatched && currentTick >= __hatchTime)
	{
		__hatched = true;
		return true;
	}
	return false;
}
