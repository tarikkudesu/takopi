#ifndef __ELEVATION_HPP__
#define __ELEVATION_HPP__

#include "CommandParser.hpp"

/*************************************************************************
 *                        ELEVATION RITUAL                               *
 *************************************************************************/

class Elevation
{
	private:
		static const s_elevation_req		__requirements[7];

		Elevation();
		Elevation(const Elevation &copy);
		Elevation						&operator=(const Elevation &assign);
		~Elevation();

	public:
		static const s_elevation_req		&getRequirement(int currentLevel);
		static bool							canElevate(int currentLevel, const Tile &tile, int sameLevelCount);
		static void							consumeStones(int currentLevel, Tile &tile);
};

#endif
