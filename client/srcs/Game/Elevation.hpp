#ifndef __ELEVATION_HPP__
#define __ELEVATION_HPP__

#include "../Enums.hpp"

/*************************************************************************
 *                      ELEVATION REQUIREMENTS TABLE                     *
 *************************************************************************/

class Elevation
{
	private:
		static const s_elevation_req		__requirements[7];

		Elevation();
		Elevation( const Elevation &copy );
		Elevation							&operator=( const Elevation &assign );
		~Elevation();

	public:
		static const s_elevation_req		&getRequirement( int currentLevel );
		static int							requiredAmount( const s_elevation_req &req, e_resource type );
};

#endif
