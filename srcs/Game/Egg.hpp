#ifndef __EGG_HPP__
#define __EGG_HPP__

#include "Player.hpp"

/*************************************************************************
 *                              EGG                                      *
 *************************************************************************/

class Egg
{
	private:
		int								__id;
		int								__x;
		int								__y;
		int								__teamIndex;
		int								__PlayerId;
		long							__hatchTime;
		bool							__hatched;

	public:
		Egg();
		Egg(int id, int x, int y, int teamIndex, int layerId, long hatchTime);
		Egg(const Egg &copy);
		Egg								&operator=(const Egg &assign);
		~Egg();

		int								getId() const;
		int								getX() const;
		int								getY() const;
		int								getTeamIndex() const;
		int								getPlayerId() const;
		long							getHatchTime() const;
		bool							isHatched() const;
		bool							tryHatch(long currentTick);
};

#endif
