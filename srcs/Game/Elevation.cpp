#include "Elevation.hpp"

Elevation::Elevation() {}
Elevation::Elevation(const Elevation &copy) { (void)copy; }
Elevation &Elevation::operator=(const Elevation &assign) { (void)assign; return *this; }
Elevation::~Elevation() {}

const s_elevation_req Elevation::__requirements[7] = {
	{1, 1, 1, 0, 0, 0, 0, 0},
	{2, 2, 1, 1, 1, 0, 0, 0},
	{3, 2, 2, 0, 1, 0, 2, 0},
	{4, 4, 1, 1, 2, 0, 1, 0},
	{5, 4, 1, 2, 1, 3, 0, 0},
	{6, 6, 1, 2, 3, 0, 1, 0},
	{7, 6, 2, 2, 2, 2, 2, 1}
};

const s_elevation_req &Elevation::getRequirement(int currentLevel)
{
	if (currentLevel < 1 || currentLevel > 7)
		return __requirements[0];
	return __requirements[currentLevel - 1];
}

bool Elevation::canElevate(int currentLevel, const Tile &tile, int sameLevelCount)
{
	if (currentLevel < 1 || currentLevel > 7)
		return false;
	const s_elevation_req &req = __requirements[currentLevel - 1];
	if (sameLevelCount < req.players_needed)
		return false;
	if (tile.getResource(LINEMATE) < req.linemate)
		return false;
	if (tile.getResource(DERAUMERE) < req.deraumere)
		return false;
	if (tile.getResource(SIBUR) < req.sibur)
		return false;
	if (tile.getResource(MENDIANE) < req.mendiane)
		return false;
	if (tile.getResource(PHIRAS) < req.phiras)
		return false;
	if (tile.getResource(THYSTAME) < req.thystame)
		return false;
	return true;
}

void Elevation::consumeStones(int currentLevel, Tile &tile)
{
	if (currentLevel < 1 || currentLevel > 7)
		return;
	const s_elevation_req &req = __requirements[currentLevel - 1];
	for (int i = 0; i < req.linemate; i++)
		tile.removeResource(LINEMATE);
	for (int i = 0; i < req.deraumere; i++)
		tile.removeResource(DERAUMERE);
	for (int i = 0; i < req.sibur; i++)
		tile.removeResource(SIBUR);
	for (int i = 0; i < req.mendiane; i++)
		tile.removeResource(MENDIANE);
	for (int i = 0; i < req.phiras; i++)
		tile.removeResource(PHIRAS);
	for (int i = 0; i < req.thystame; i++)
		tile.removeResource(THYSTAME);
}
