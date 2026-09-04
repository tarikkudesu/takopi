#include "Strategy.hpp"
#include "../utilities/MZU.hpp"

static const int RESOURCE_STOCKPILE_CAP = 10;
static const e_resource STONES[] = {LINEMATE, DERAUMERE, SIBUR, MENDIANE, PHIRAS, THYSTAME};
static const size_t STONE_COUNT = sizeof(STONES) / sizeof(STONES[0]);

Strategy::Strategy() : __level(1), __hasInventory(false), __hasVision(false),
						__ticksSinceInventory(INVENTORY_POLL_INTERVAL)
{
	for (int i = 0; i < RESOURCE_COUNT; i++)
		__inventory[i] = 0;
}
Strategy::Strategy(const Strategy &copy)
{
	*this = copy;
}
Strategy &Strategy::operator=(const Strategy &assign)
{
	if (this != &assign)
	{
		for (int i = 0; i < RESOURCE_COUNT; i++)
			__inventory[i] = assign.__inventory[i];
		__level = assign.__level;
		__hasInventory = assign.__hasInventory;
		__hasVision = assign.__hasVision;
		__ticksSinceInventory = assign.__ticksSinceInventory;
		__vision = assign.__vision;
	}
	return *this;
}
Strategy::~Strategy()
{
}

int Strategy::getLevel() const
{
	return __level;
}

/****************************************************************************
 *								STATE UPDATES								*
 ****************************************************************************/

void Strategy::applyVision(const String &rawLine)
{
	__vision = Protocol::parseVision(rawLine);
	__hasVision = !__vision.empty();
}

void Strategy::invalidateVision()
{
	__hasVision = false;
}

void Strategy::applyInventory(const String &rawLine)
{
	if (Protocol::parseInventory(rawLine, __inventory))
	{
		__hasInventory = true;
		__ticksSinceInventory = 0;
	}
}

void Strategy::applyLevel(int level)
{
	__level = level;
	mzu::info("level up: now level " + mzu::intToString(level));
}

void Strategy::applyPrendResult(e_resource resource, bool ok)
{
	if (!ok)
	{
		invalidateVision();
		return;
	}
	__inventory[resource]++;
	if (__hasVision && !__vision.empty() && __vision[0].resources[resource] > 0)
		__vision[0].resources[resource]--;
}

void Strategy::applyPoseResult(e_resource resource, bool ok)
{
	if (!ok)
		return;
	if (__inventory[resource] > 0)
		__inventory[resource]--;
	if (__hasVision && !__vision.empty())
		__vision[0].resources[resource]++;
}

/****************************************************************************
 *							ELEVATION HELPERS								*
 ****************************************************************************/

bool Strategy::tileHasEnough(const t_tile_content &tile, const s_elevation_req &req) const
{
	for (size_t i = 0; i < STONE_COUNT; i++)
	{
		if (tile.resources[STONES[i]] < Elevation::requiredAmount(req, STONES[i]))
			return false;
	}
	return true;
}

e_resource Strategy::firstUnsatisfiedNeed(const t_tile_content &tile, const s_elevation_req &req) const
{
	for (size_t i = 0; i < STONE_COUNT; i++)
	{
		if (tile.resources[STONES[i]] < Elevation::requiredAmount(req, STONES[i]))
			return STONES[i];
	}
	return RESOURCE_COUNT;
}

e_resource Strategy::anyStockpileableStone(const t_tile_content &tile, const s_elevation_req &req) const
{
	for (size_t i = 0; i < STONE_COUNT; i++)
	{
		/* Only take genuine surplus: a stone this level still needs must be
		 * left in place once the ground holds just enough of it, otherwise
		 * we'd pick up and re-lay the same pile forever while some other
		 * requirement stays unmet. */
		if (tile.resources[STONES[i]] > Elevation::requiredAmount(req, STONES[i])
			&& __inventory[STONES[i]] < RESOURCE_STOCKPILE_CAP)
			return STONES[i];
	}
	return RESOURCE_COUNT;
}

int Strategy::findNearestTileWithResource(e_resource type) const
{
	for (size_t i = 1; i < __vision.size(); i++)
	{
		if (__vision[i].resources[type] > 0)
			return static_cast<int>(i);
	}
	return -1;
}

/****************************************************************************
 *								MOVEMENT										*
 ****************************************************************************/

/* The vision string lists tile 0 (ourselves), then each ring of the field of
 * view outward; ring i holds tiles (2i+1) wide, so ring i starts at flat
 * index i*i. Recovering (ring, side-offset) from a flat index tells us
 * whether the target is dead ahead or off to one side. */
String Strategy::stepToward(size_t tileIndex) const
{
	int idx = static_cast<int>(tileIndex);
	int ring = static_cast<int>(std::sqrt(static_cast<double>(idx)));
	while (ring * ring > idx)
		ring--;
	while ((ring + 1) * (ring + 1) <= idx)
		ring++;
	int side = (idx - ring * ring) - ring;

	if (side < 0)
		return "gauche";
	if (side > 0)
		return "droite";
	return "avance";
}

String Strategy::wander() const
{
	if (rand() % 6 == 0)
		return (rand() % 2 == 0) ? "droite" : "gauche";
	return "avance";
}

/****************************************************************************
 *								DECISION MAKING								*
 ****************************************************************************/

String Strategy::decideNextCommand()
{
	if (!__hasVision)
		return "voir";
	if (!__hasInventory)
		return "inventaire";
	if (__ticksSinceInventory >= INVENTORY_POLL_INTERVAL)
	{
		__ticksSinceInventory = 0;
		return "inventaire";
	}
	__ticksSinceInventory++;

	const t_tile_content &here = __vision[0];

	if (here.resources[NOURRITURE] > 0 && __inventory[NOURRITURE] < RESOURCE_STOCKPILE_CAP)
		return "prend " + Protocol::resourceName(NOURRITURE);

	if (__inventory[NOURRITURE] < FOOD_SAFETY_THRESHOLD)
	{
		int target = findNearestTileWithResource(NOURRITURE);
		if (target > 0)
			return stepToward(static_cast<size_t>(target));
		return wander();
	}

	if (__level < WIN_LEVEL)
	{
		const s_elevation_req &req = Elevation::getRequirement(__level);

		if (tileHasEnough(here, req))
		{
			if (here.players >= req.players_needed)
				return "incantation";
			/* Resources are already piled up; just watch for teammates to
			 * join rather than picking the pile back apart. */
			return "voir";
		}

		e_resource need = firstUnsatisfiedNeed(here, req);
		if (need != RESOURCE_COUNT)
		{
			if (__inventory[need] > 0)
				return "pose " + Protocol::resourceName(need);
			int target = findNearestTileWithResource(need);
			if (target > 0)
				return stepToward(static_cast<size_t>(target));
		}

		e_resource stray = anyStockpileableStone(here, req);
		if (stray != RESOURCE_COUNT)
			return "prend " + Protocol::resourceName(stray);
	}

	return wander();
}
