#include "Protocol.hpp"
#include "../utilities/MZU.hpp"

Protocol::Protocol() {}
Protocol::Protocol(const Protocol &copy) { (void)copy; }
Protocol &Protocol::operator=(const Protocol &assign) { (void)assign; return *this; }
Protocol::~Protocol() {}

/****************************************************************************
 *								RESOURCE NAMES								*
 ****************************************************************************/

String Protocol::resourceName(e_resource res)
{
	switch (res)
	{
		case NOURRITURE:	return "nourriture";
		case LINEMATE:		return "linemate";
		case DERAUMERE:		return "deraumere";
		case SIBUR:			return "sibur";
		case MENDIANE:		return "mendiane";
		case PHIRAS:		return "phiras";
		case THYSTAME:		return "thystame";
		default:			return "";
	}
}

e_resource Protocol::resourceFromName(const String &name)
{
	if (name == "nourriture")	return NOURRITURE;
	if (name == "linemate")		return LINEMATE;
	if (name == "deraumere")	return DERAUMERE;
	if (name == "sibur")		return SIBUR;
	if (name == "mendiane")		return MENDIANE;
	if (name == "phiras")		return PHIRAS;
	if (name == "thystame")		return THYSTAME;
	return RESOURCE_COUNT;
}

/****************************************************************************
 *								VISION / VOIR								*
 ****************************************************************************/

t_tile_content Protocol::parseTileContent(const String &tileStr)
{
	t_tile_content content;

	content.players = 0;
	for (int i = 0; i < RESOURCE_COUNT; i++)
		content.resources[i] = 0;

	t_svec tokens = mzu::splitBySpaces(tileStr);
	for (t_svec::iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		if (*it == "player")
		{
			content.players++;
			continue;
		}
		e_resource res = resourceFromName(*it);
		if (res != RESOURCE_COUNT)
			content.resources[res]++;
	}
	return content;
}

std::vector<t_tile_content> Protocol::parseVision(const String &line)
{
	std::vector<t_tile_content> tiles;
	String body = line;

	mzu::trimSpaces(body);
	if (body.size() >= 2 && body[0] == '{' && body[body.size() - 1] == '}')
		body = body.substr(1, body.size() - 2);

	t_svec cases = mzu::splitByChar(body, ',');
	for (t_svec::iterator it = cases.begin(); it != cases.end(); it++)
	{
		mzu::trimSpaces(*it);
		tiles.push_back(parseTileContent(*it));
	}
	return tiles;
}

/****************************************************************************
 *								INVENTAIRE									*
 ****************************************************************************/

bool Protocol::parseInventory(const String &line, int inventory[RESOURCE_COUNT])
{
	String body = line;

	mzu::trimSpaces(body);
	if (body.size() < 2 || body[0] != '{' || body[body.size() - 1] != '}')
		return false;
	body = body.substr(1, body.size() - 2);

	t_svec entries = mzu::splitByChar(body, ',');
	for (t_svec::iterator it = entries.begin(); it != entries.end(); it++)
	{
		mzu::trimSpaces(*it);
		t_svec pair = mzu::splitBySpaces(*it);
		if (pair.size() != 2)
			continue;
		e_resource res = resourceFromName(pair[0]);
		if (res != RESOURCE_COUNT)
			inventory[res] = static_cast<int>(mzu::stringToInt(pair[1]));
	}
	return true;
}

/****************************************************************************
 *							UNSOLICITED NOTIFICATIONS						*
 ****************************************************************************/

bool Protocol::parseBroadcast(const String &line, int &outDirection, String &outText)
{
	static const String prefix = "message ";

	if (line.compare(0, prefix.length(), prefix) != 0)
		return false;
	size_t comma = line.find(',');
	if (comma == String::npos)
		return false;
	outDirection = static_cast<int>(mzu::stringToInt(line.substr(prefix.length(), comma - prefix.length())));
	outText = line.substr(comma + 1);
	return true;
}

bool Protocol::parseDeplacement(const String &line, int &outDirection)
{
	static const String prefix = "deplacement ";

	if (line.compare(0, prefix.length(), prefix) != 0)
		return false;
	outDirection = static_cast<int>(mzu::stringToInt(line.substr(prefix.length())));
	return true;
}

bool Protocol::parseNiveauActuel(const String &line, int &outLevel)
{
	static const String prefix = "niveau actuel : ";

	if (line.compare(0, prefix.length(), prefix) != 0)
		return false;
	outLevel = static_cast<int>(mzu::stringToInt(line.substr(prefix.length())));
	return true;
}

bool Protocol::isDeath(const String &line)
{
	return line == "mort";
}

bool Protocol::isElevationEnCours(const String &line)
{
	return line == "elevation en cours";
}
