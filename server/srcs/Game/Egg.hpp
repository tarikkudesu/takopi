#ifndef __EGG_HPP__
#define __EGG_HPP__

#include "../zappy.hpp"

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
		int								__parentPlayerId;
		long							__hatchTime;
		t_egg_state						__state;

	public:
		Egg();
		Egg(int id, int parentPlayerId, int x, int y, int teamIndex, long hatchTime);
		Egg(const Egg &copy);
		Egg	&operator=(const Egg &assign);
		~Egg();

		int								getX() const;
		int								getY() const;
		int								getId() const;
		t_egg_state						getState() const;
		bool							isHatched() const;
		int								getTeamIndex() const;
		int								getParentPlayerId() const;
		void							setPosition(int x, int y);
		bool							hatchIfReady(long currentTick);
};

#endif
