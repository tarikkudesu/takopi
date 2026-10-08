#include "Strategy.hpp"
#include "../utilities/MZU.hpp"
#include <cstdlib>
#include <ctime>
#include <cmath>

static const e_resource STONES[] = {LINEMATE, DERAUMERE, SIBUR, MENDIANE, PHIRAS, THYSTAME};
static const size_t STONE_COUNT = sizeof(STONES) / sizeof(STONES[0]);

Strategy::Strategy() : __level(1), __hasInventory(false), __hasVision(false),
						__ticksSinceInventory(999), __teamName(""), __lastBaseDirection(-1),
						__broadcastTimer(0), __phase(0)
{
	for (int i = 0; i < RESOURCE_COUNT; i++)
		__inventory[i] = 0;
	srand(time(NULL) ^ (long)this);
	__myId = rand() % 1000000;
	__baseId = __myId;
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
		__teamName = assign.__teamName;
		__myId = assign.__myId;
		__baseId = assign.__baseId;
		__lastBaseDirection = assign.__lastBaseDirection;
		__broadcastTimer = assign.__broadcastTimer;
		__phase = assign.__phase;
	}
	return *this;
}

Strategy::~Strategy()
{
}

void Strategy::setTeamName(const String &team)
{
	__teamName = team;
}

int Strategy::getLevel() const
{
	return __level;
}

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
	if (!ok) { invalidateVision(); return; }
	__inventory[resource]++;
	if (__hasVision && !__vision.empty() && __vision[0].resources[resource] > 0)
		__vision[0].resources[resource]--;
}

void Strategy::applyPoseResult(e_resource resource, bool ok)
{
	if (!ok) return;
	if (__inventory[resource] > 0) __inventory[resource]--;
	if (__hasVision && !__vision.empty()) __vision[0].resources[resource]++;
}

void Strategy::applyBroadcast(int direction, const String &text)
{
	t_svec tokens = mzu::splitBySpaces(text);
	if (tokens.size() >= 2 && tokens[0] == __teamName)
	{
		int id = mzu::stringToInt(tokens[1]);
		if (id > __baseId)
		{
			__baseId = id;
			__lastBaseDirection = direction;
		}
		else if (id == __baseId)
		{
			__lastBaseDirection = direction;
		}
	}
}

bool Strategy::tileHasEnough(const t_tile_content &tile, const s_elevation_req &req) const
{
	for (size_t i = 0; i < STONE_COUNT; i++)
	{
		if (tile.resources[STONES[i]] < Elevation::requiredAmount(req, STONES[i]))
			return false;
	}
	return true;
}

String Strategy::stepToward(size_t tileIndex) const
{
	int idx = static_cast<int>(tileIndex);
	int ring = static_cast<int>(std::sqrt(static_cast<double>(idx)));
	while (ring * ring > idx) ring--;
	while ((ring + 1) * (ring + 1) <= idx) ring++;
	int side = (idx - ring * ring) - ring;

	if (side < 0) return "gauche";
	if (side > 0) return "droite";
	return "avance";
}

String Strategy::stepDirection(int direction) const
{
	if (direction == 1) return "avance";
	if (direction == 2 || direction == 3 || direction == 4) return "gauche";
	if (direction == 5) return "gauche"; // Turn around
	if (direction == 6 || direction == 7 || direction == 8) return "droite";
	return "avance";
}

String Strategy::wander() const
{
	if (rand() % 6 == 0) return (rand() % 2 == 0) ? "droite" : "gauche";
	return "avance";
}

String Strategy::decideNextCommand()
{
	if (!__hasVision) return "voir";
	if (!__hasInventory || __ticksSinceInventory >= 20)
	{
		if (__hasInventory) __ticksSinceInventory = 0;
		return "inventaire";
	}
	__ticksSinceInventory++;

	if (__level >= WIN_LEVEL) return wander();

	// Broadcast periodically
	if (++__broadcastTimer >= 3)
	{
		__broadcastTimer = 0;
		return "broadcast " + __teamName + " " + mzu::intToString(__myId);
	}

	const t_tile_content &here = __vision[0];

	// Survival override
	if (__inventory[NOURRITURE] < 10 && here.resources[NOURRITURE] > 0)
		return "prend " + Protocol::resourceName(NOURRITURE);

	// Phase management
	if (__phase == 0 && __inventory[NOURRITURE] >= 35)
		__phase = 1;
	else if (__phase == 1 && __inventory[NOURRITURE] < 15)
		__phase = 0;

	if (__phase == 0)
	{
		// FORAGE phase: pick up food and stones
		if (here.resources[NOURRITURE] > 0)
			return "prend " + Protocol::resourceName(NOURRITURE);
		for (size_t i = 0; i < STONE_COUNT; i++)
		{
			if (here.resources[STONES[i]] > 0)
				return "prend " + Protocol::resourceName(STONES[i]);
		}
		// Move to nearest food if visible
		for (size_t i = 1; i < __vision.size(); i++)
		{
			if (__vision[i].resources[NOURRITURE] > 0)
				return stepToward(i);
		}
		return wander();
	}
	else
	{
		// GROUP phase
		bool isBase = (__myId == __baseId);

		if (isBase)
		{
			// Base behavior: check incantation
			const s_elevation_req &req = Elevation::getRequirement(__level);
			if (here.players >= req.players_needed)
			{
				if (tileHasEnough(here, req))
					return "incantation";
			}
			
			// Drop all stones on the ground
			for (size_t i = 0; i < STONE_COUNT; i++)
			{
				if (__inventory[STONES[i]] > 0)
					return "pose " + Protocol::resourceName(STONES[i]);
			}
			
			// Stay alive and take dropped food from workers
			if (here.resources[NOURRITURE] > 0 && __inventory[NOURRITURE] < 80)
				return "prend " + Protocol::resourceName(NOURRITURE);

			return "voir"; // Just wait
		}
		else
		{
			// Worker behavior: go to base
			if (__lastBaseDirection > 0)
			{
				String cmd = stepDirection(__lastBaseDirection);
				if (cmd == "gauche")
				{
					__lastBaseDirection -= 2;
					if (__lastBaseDirection <= 0) __lastBaseDirection += 8;
				}
				else if (cmd == "droite")
				{
					__lastBaseDirection += 2;
					if (__lastBaseDirection > 8) __lastBaseDirection -= 8;
				}
				else if (cmd == "avance")
				{
					// If we advance, we shouldn't necessarily keep advancing blindly forever
					// Invalidate it so we wait for the next broadcast, which is very frequent (every 3 actions)
					__lastBaseDirection = -1;
				}
				return cmd;
			}
			else if (__lastBaseDirection == 0)
			{
				// At base: drop all stones
				for (size_t i = 0; i < STONE_COUNT; i++)
				{
					if (__inventory[STONES[i]] > 0)
						return "pose " + Protocol::resourceName(STONES[i]);
				}
				
				// Drop excess food to feed base
				if (__inventory[NOURRITURE] > 25)
				{
					return "pose " + Protocol::resourceName(NOURRITURE);
				}
				
				// Just wait at base for incantation
				return "voir";
			}
			else
			{
				// No base known yet, just wait or wander?
				return wander();
			}
		}
	}
}
