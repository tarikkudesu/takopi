#ifndef __STRATEGY_HPP__
#define __STRATEGY_HPP__

#include "Protocol.hpp"
#include "Elevation.hpp"

/*************************************************************************
 *                    REACTIVE SURVIVE-AND-ELEVATE AI                    *
 *************************************************************************/

class Strategy
{
	private:
		int								__inventory[RESOURCE_COUNT];
		int								__level;
		bool							__hasInventory;
		bool							__hasVision;
		int								__ticksSinceInventory;
		std::vector<t_tile_content>	__vision;

		String							wander() const;
		String							stepToward( size_t tileIndex ) const;
		bool							tileHasEnough( const t_tile_content &tile, const s_elevation_req &req ) const;
		e_resource						firstUnsatisfiedNeed( const t_tile_content &tile, const s_elevation_req &req ) const;
		e_resource						anyStockpileableStone( const t_tile_content &tile, const s_elevation_req &req ) const;
		int								findNearestTileWithResource( e_resource type ) const;

	public:
		Strategy();
		Strategy( const Strategy &copy );
		Strategy						&operator=( const Strategy &assign );
		~Strategy();

		String							decideNextCommand();

		void							applyVision( const String &rawLine );
		void							applyInventory( const String &rawLine );
		void							applyLevel( int level );
		void							applyPrendResult( e_resource resource, bool ok );
		void							applyPoseResult( e_resource resource, bool ok );
		void							invalidateVision();

		int								getLevel() const;
};

#endif
