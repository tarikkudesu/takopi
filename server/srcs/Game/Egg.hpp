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
		long							__hatchTime;

	public:
		Egg();
		Egg(int id, int x, int y, int teamIndex, long hatchTime);
		Egg(const Egg &copy);
		Egg								&operator=(const Egg &assign);
		~Egg();

		int								getX() const;
		int								getY() const;
		int								getId() const;
		int								getTeamIndex() const;
		void							setPosition(int x, int y);
		bool							tryHatch(long currentTick);
};

#endif
