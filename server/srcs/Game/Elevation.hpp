#ifndef __ELEVATION_HPP__
#define __ELEVATION_HPP__

#include "Tile.hpp"

/*************************************************************************
 *                        ELEVATION RITUAL                               *
 *************************************************************************/

class Elevation
{
	private:
		static const s_elevation_req		__requirements[7];

		Elevation() = delete;
		Elevation(const Elevation &copy) = delete;
		Elevation						&operator=(const Elevation &assign) = delete;
		~Elevation() = delete;

	public:
		static bool							canElevate(int currentLevel, const Tile &tile, int sameLevelCount);
		static void							consumeStones(int currentLevel, Tile &tile);
};

#endif
