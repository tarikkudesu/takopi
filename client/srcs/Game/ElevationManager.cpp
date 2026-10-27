#include "ElevationManager.hpp"

static const e_resource ALL_STONES[] = {LINEMATE, DERAUMERE, SIBUR, MENDIANE, PHIRAS, THYSTAME};
static const size_t ALL_STONES_COUNT = sizeof(ALL_STONES) / sizeof(ALL_STONES[0]);

void ElevationManager::calculateMissingStones(int currentLevel, const t_tile_content &tile, int outMissing[RESOURCE_COUNT])
{
	for (int i = 0; i < RESOURCE_COUNT; i++)
		outMissing[i] = 0;

	if (currentLevel < 1 || currentLevel >= WIN_LEVEL)
		return;

	const s_elevation_req &req = Elevation::getRequirement(currentLevel);
	for (size_t i = 0; i < ALL_STONES_COUNT; i++)
	{
		e_resource res = ALL_STONES[i];
		int needed = Elevation::requiredAmount(req, res);
		int onTile = tile.resources[res];
		if (onTile < needed)
			outMissing[res] = needed - onTile;
	}
}

bool ElevationManager::isTileResourceComplete(int currentLevel, const t_tile_content &tile)
{
	int missing[RESOURCE_COUNT];
	calculateMissingStones(currentLevel, tile, missing);
	for (size_t i = 0; i < ALL_STONES_COUNT; i++)
	{
		if (missing[ALL_STONES[i]] > 0)
			return false;
	}
	return true;
}

bool ElevationManager::isTilePlayersComplete(int currentLevel, const t_tile_content &tile)
{
	if (currentLevel < 1 || currentLevel >= WIN_LEVEL)
		return false;
	const s_elevation_req &req = Elevation::getRequirement(currentLevel);
	return tile.players == req.players_needed;
}

bool ElevationManager::canContribute(const int inventory[RESOURCE_COUNT], const int missingStones[RESOURCE_COUNT])
{
	for (size_t i = 0; i < ALL_STONES_COUNT; i++)
	{
		e_resource res = ALL_STONES[i];
		if (missingStones[res] > 0 && inventory[res] > 0)
			return true;
	}
	return false;
}

bool ElevationManager::hasAllElevationStones(int currentLevel, const int inventory[RESOURCE_COUNT])
{
	if (currentLevel < 1 || currentLevel >= WIN_LEVEL)
		return false;
	const s_elevation_req &req = Elevation::getRequirement(currentLevel);
	for (size_t i = 0; i < ALL_STONES_COUNT; i++)
	{
		e_resource res = ALL_STONES[i];
		int needed = Elevation::requiredAmount(req, res);
		if (inventory[res] < needed)
			return false;
	}
	return true;
}

bool ElevationManager::hasEnoughToLead(int currentLevel, const int inventory[RESOURCE_COUNT])
{
	if (currentLevel < 1 || currentLevel >= WIN_LEVEL)
		return false;
	const s_elevation_req &req = Elevation::getRequirement(currentLevel);
	int totalNeeded = 0;
	int totalPresent = 0;
	for (size_t i = 0; i < ALL_STONES_COUNT; i++)
	{
		e_resource res = ALL_STONES[i];
		int needed = Elevation::requiredAmount(req, res);
		totalNeeded += needed;
		int have = inventory[res];
		if (have > needed)
			have = needed;
		totalPresent += have;
	}
	if (totalNeeded <= 0)
		return false;

	// Only lead if holding at least 75% of the total required stones
	double stoneRatio = static_cast<double>(totalPresent) / static_cast<double>(totalNeeded);
	if (stoneRatio <= 0.75)
		return false;

	// Rare stone prerequisites per level
	if (currentLevel == 3 && (inventory[SIBUR] < 1 || inventory[PHIRAS] < 1))
		return false;
	if (currentLevel == 4 && (inventory[SIBUR] < 1 || inventory[PHIRAS] < 1))
		return false;
	if (currentLevel == 5 && inventory[MENDIANE] < 2)
		return false;
	if (currentLevel == 6 && (inventory[SIBUR] < 2 || inventory[PHIRAS] < 1))
		return false;
	if (currentLevel == 7 && inventory[THYSTAME] < 1)
		return false;

	return true;
}

e_resource ElevationManager::getStoneToDeposit(const int inventory[RESOURCE_COUNT], const int missingStones[RESOURCE_COUNT])
{
	for (size_t i = 0; i < ALL_STONES_COUNT; i++)
	{
		e_resource res = ALL_STONES[i];
		if (missingStones[res] > 0 && inventory[res] > 0)
			return res;
	}
	return RESOURCE_COUNT;
}
