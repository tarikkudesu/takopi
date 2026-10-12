#ifndef __ELEVATION_MANAGER_HPP__
#define __ELEVATION_MANAGER_HPP__

#include "Elevation.hpp"

class ElevationManager
{
	public:
		// Calculates missing stones on a given tile compared to requirement
		static void calculateMissingStones( int currentLevel, const t_tile_content &tile, int outMissing[RESOURCE_COUNT] );

		// Checks if a tile has all required resources for elevation
		static bool isTileResourceComplete( int currentLevel, const t_tile_content &tile );

		// Checks if tile has enough players of the same level (at least req.players_needed)
		static bool isTilePlayersComplete( int currentLevel, const t_tile_content &tile );

		// Checks if a player's inventory contains at least one stone needed in missingStones
		static bool canContribute( const int inventory[RESOURCE_COUNT], const int missingStones[RESOURCE_COUNT] );

		// Checks if an inventory has ALL stones required for the given level elevation
		static bool hasAllElevationStones( int currentLevel, const int inventory[RESOURCE_COUNT] );

		// Checks if an inventory has enough stones to assume leader (allowing workers to pool remaining stones)
		static bool hasEnoughToLead( int currentLevel, const int inventory[RESOURCE_COUNT] );

		// Returns the first resource in inventory that is needed on the tile
		static e_resource getStoneToDeposit( const int inventory[RESOURCE_COUNT], const int missingStones[RESOURCE_COUNT] );
};

#endif
