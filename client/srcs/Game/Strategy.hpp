#ifndef __STRATEGY_HPP__
#define __STRATEGY_HPP__

#include "Protocol.hpp"
#include "Elevation.hpp"

class Strategy
{
	private:
		int								__inventory[RESOURCE_COUNT];
		int								__level;
		bool							__hasInventory;
		bool							__hasVision;
		int								__ticksSinceInventory;
		std::vector<t_tile_content>	__vision;

		String							__teamName;
		int								__myId;
		int								__baseId;
		int								__lastBaseDirection;
		int								__broadcastTimer;
		int								__phase; // 0 = FORAGE, 1 = GROUP
		int								__stonesToDrop; // bitmask or just state

		String							wander() const;
		String							stepToward( size_t tileIndex ) const;
		String							stepDirection( int direction ) const;
		bool							tileHasEnough( const t_tile_content &tile, const s_elevation_req &req ) const;

	public:
		Strategy();
		Strategy( const Strategy &copy );
		Strategy						&operator=( const Strategy &assign );
		~Strategy();

		void							setTeamName(const String &team);
		String							decideNextCommand();

		void							applyVision( const String &rawLine );
		void							applyInventory( const String &rawLine );
		void							applyLevel( int level );
		void							applyPrendResult( e_resource resource, bool ok );
		void							applyPoseResult( e_resource resource, bool ok );
		void							invalidateVision();
		void							applyBroadcast( int direction, const String &text );

		int								getLevel() const;
};

#endif
