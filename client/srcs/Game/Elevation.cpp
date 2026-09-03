#include "Elevation.hpp"

Elevation::Elevation() {}
Elevation::Elevation(const Elevation &copy) { (void)copy; }
Elevation &Elevation::operator=(const Elevation &assign) { (void)assign; return *this; }
Elevation::~Elevation() {}

/* Kept in lockstep with server/srcs/Game/Elevation.cpp: the client has to reason
 * about the exact same requirement table the server enforces to know when an
 * incantation is worth attempting. */
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

int Elevation::requiredAmount(const s_elevation_req &req, e_resource type)
{
	switch (type)
	{
		case LINEMATE:		return req.linemate;
		case DERAUMERE:		return req.deraumere;
		case SIBUR:			return req.sibur;
		case MENDIANE:		return req.mendiane;
		case PHIRAS:		return req.phiras;
		case THYSTAME:		return req.thystame;
		default:			return 0;
	}
}
