#include "Protocole.hpp"
#include <limits>

static const e_resource g_guiResourceOrder[RESOURCE_COUNT] =
{
	NOURRITURE, LINEMATE, DERAUMERE, SIBUR,
	MENDIANE, PHIRAS, THYSTAME
};

static const char *g_protocolVerbs[] =
{
	"BIENVENUE", "GRAPHIC", "msz", "bct", "mct", "tna", "pnw", "ppo",
	"plv", "pin", "pex", "pbc", "pic", "pie", "pfk", "pdr", "pgt",
	"pdi", "enw", "eht", "ebo", "edi", "sgt", "sst", "seg", "smg",
	"suc", "sbp", ""
};

t_protocol_verb Protocole::parseVerb(const String &value)
{
	for (int current = PROTOCOL_WELCOME; current < PROTOCOL_UNKNOWN; current++)
	{
		if (value == g_protocolVerbs[current])
			return static_cast<t_protocol_verb>(current);
	}
	return PROTOCOL_UNKNOWN;
}

String Protocole::verb(t_protocol_verb value)
{
	if (value < PROTOCOL_WELCOME || value > PROTOCOL_UNKNOWN)
		return "";
	return g_protocolVerbs[value];
}

bool Protocole::parseUnsigned(const String &token, int &value)
{
	if (token.empty() || token.find_first_not_of("0123456789") != String::npos)
		return false;
	std::istringstream input(token);
	long parsed;
	if (!(input >> parsed) || parsed > std::numeric_limits<int>::max())
		return false;
	value = static_cast<int>(parsed);
	return true;
}

bool Protocole::parsePlayerId(const String &token, int &playerId)
{
	return token.length() > 1 && token[0] == '#' && parseUnsigned(token.substr(1), playerId);
}

int Protocole::protocolOrientation(e_direction direction)
{
	return static_cast<int>(direction) + 1;
}

int Protocole::protocolResource(e_resource resource)
{
	for (int i = 0; i < RESOURCE_COUNT; i++)
	{
		if (g_guiResourceOrder[i] == resource)
			return i;
	}
	return -1;
}

String Protocole::welcome()
{
	return verb(PROTOCOL_WELCOME) + NEWLINE;
}

String Protocole::mapSize(const Game &game)
{
	return verb(PROTOCOL_MSZ) + " " + mzu::intToString(game.getWorld().getWidth()) + " " + mzu::intToString(game.getWorld().getHeight()) + NEWLINE;
}

String Protocole::tile(const Game &game, int x, int y)
{
	const World &world = game.getWorld();
	if (x < 0 || y < 0 || x >= world.getWidth() || y >= world.getHeight())
		return badParameters();
	const Tile &current = world.tileAt(x, y);
	String result = verb(PROTOCOL_BCT) + " " + mzu::intToString(x) + " " + mzu::intToString(y);
	for (int i = 0; i < RESOURCE_COUNT; i++)
		result += " " + mzu::intToString(current.getResource(g_guiResourceOrder[i]));
	return result + NEWLINE;
}

String Protocole::fullMap(const Game &game)
{
	String result;
	for (int y = 0; y < game.getWorld().getHeight(); y++)
	{
		for (int x = 0; x < game.getWorld().getWidth(); x++)
			result += tile(game, x, y);
	}
	return result;
}

String Protocole::teams(const Game &game)
{
	String result;
	const t_svec &names = game.getTeams();
	for (size_t i = 0; i < names.size(); i++)
		result += verb(PROTOCOL_TNA) + " " + names[i] + NEWLINE;
	return result;
}

String Protocole::timeUnit(const Game &game)
{
	return verb(PROTOCOL_SGT) + " " + mzu::intToString(game.getTimeUnit()) + NEWLINE;
}

String Protocole::playerNew(const Game &game, const Player &player)
{
	return verb(PROTOCOL_PNW) + " #" + mzu::intToString(player.getId()) + " "
		+ mzu::intToString(player.getX()) + " " + mzu::intToString(player.getY()) + " "
		+ mzu::intToString(protocolOrientation(player.getDirection())) + " "
		+ mzu::intToString(player.getLevel()) + " "
		+ game.getTeamName(player.getTeamIndex()) + NEWLINE;
}

String Protocole::playerPosition(const Player &player)
{
	return verb(PROTOCOL_PPO) + " #" + mzu::intToString(player.getId()) + " " + mzu::intToString(player.getX()) + " " + mzu::intToString(player.getY()) + " " + mzu::intToString(protocolOrientation(player.getDirection())) + NEWLINE;
}

String Protocole::playerLevel(const Player &player)
{
	return verb(PROTOCOL_PLV) + " #" + mzu::intToString(player.getId()) + " " + mzu::intToString(player.getLevel()) + NEWLINE;
}

String Protocole::playerInventory(const Player &player)
{
	String result = verb(PROTOCOL_PIN) + " #" + mzu::intToString(player.getId()) + " " + mzu::intToString(player.getX()) + " " + mzu::intToString(player.getY());
	for (int i = 0; i < RESOURCE_COUNT; i++)
		result += " " + mzu::intToString(player.getInventory(g_guiResourceOrder[i]));
	return result + NEWLINE;
}

String Protocole::eggNew(const Egg &egg)
{
	return verb(PROTOCOL_ENW) + " #" + mzu::intToString(egg.getId()) + " #" + mzu::intToString(egg.getParentPlayerId()) + " " + mzu::intToString(egg.getX()) + " " + mzu::intToString(egg.getY()) + NEWLINE;
}

String Protocole::snapshot(const Game &game)
{
	String result = mapSize(game) + timeUnit(game) + fullMap(game) + teams(game);
	const std::map<int, Player *> &players = game.getPlayers();
	for (std::map<int, Player *>::const_iterator it = players.begin(); it != players.end(); it++)
	{
		if (it->second && it->second->isAlive())
			result += playerNew(game, *it->second);
	}
	const std::vector<Egg *> &eggs = game.getEggs();
	for (size_t i = 0; i < eggs.size(); i++)
	{
		result += eggNew(*eggs[i]);
		if (eggs[i]->isHatched())
			result += eggHatched(eggs[i]->getId());
	}
	return result;
}

String Protocole::executeRequest(const String &request, Game &game)
{
	t_svec args = mzu::splitBySpaces(request);
	if (args.empty())
		return unknownCommand();
	t_protocol_verb command = parseVerb(args[0]);
	if (command == PROTOCOL_MSZ || command == PROTOCOL_MCT
		|| command == PROTOCOL_TNA || command == PROTOCOL_SGT)
	{
		if (args.size() != 1)
			return badParameters();
		if (command == PROTOCOL_MSZ) return mapSize(game);
		if (command == PROTOCOL_MCT) return fullMap(game);
		if (command == PROTOCOL_TNA) return teams(game);
		return timeUnit(game);
	}
	if (command == PROTOCOL_BCT)
	{
		int x, y;
		if (args.size() != 3 || !parseUnsigned(args[1], x) || !parseUnsigned(args[2], y)
			|| x >= game.getWorld().getWidth() || y >= game.getWorld().getHeight())
			return badParameters();
		return tile(game, x, y);
	}
	if (command == PROTOCOL_PPO || command == PROTOCOL_PLV || command == PROTOCOL_PIN)
	{
		int playerId;
		if (args.size() != 2 || !parsePlayerId(args[1], playerId))
			return badParameters();
		const Player *player = static_cast<const Game &>(game).getPlayer(playerId);
		if (!player || !player->isAlive())
			return badParameters();
		if (command == PROTOCOL_PPO) return playerPosition(*player);
		if (command == PROTOCOL_PLV) return playerLevel(*player);
		return playerInventory(*player);
	}
	if (command == PROTOCOL_SST)
	{
		int value;
		if (args.size() != 2 || !parseUnsigned(args[1], value) || game.getState() != GAME_RUNNING)
			return badParameters();
		try { game.setTimeUnit(value); }
		catch (const std::exception &) { return badParameters(); }
		return "";
	}
	return unknownCommand();
}

String Protocole::resourceDropped(int playerId, e_resource resource)
{
	return verb(PROTOCOL_PDR) + " #" + mzu::intToString(playerId) + " " + mzu::intToString(protocolResource(resource)) + NEWLINE;
}

String Protocole::resourceTaken(int playerId, e_resource resource)
{
	return verb(PROTOCOL_PGT) + " #" + mzu::intToString(playerId) + " " + mzu::intToString(protocolResource(resource)) + NEWLINE;
}

String Protocole::playerBroadcast(int playerId, const String &message)
{
	return verb(PROTOCOL_PBC) + " #" + mzu::intToString(playerId) + " " + message + NEWLINE;
}

String Protocole::playerExpelled(int playerId)
{ 
	return verb(PROTOCOL_PEX) + " #" + mzu::intToString(playerId) + NEWLINE;
}

String Protocole::playerDeath(int playerId)
{ 
	return verb(PROTOCOL_PDI) + " #" + mzu::intToString(playerId) + NEWLINE;
}

String Protocole::forkStart(int playerId)
{ 
	return verb(PROTOCOL_PFK) + " #" + mzu::intToString(playerId) + NEWLINE;
}

String Protocole::eggConsumed(int eggId)
{ 
	return verb(PROTOCOL_EBO) + " #" + mzu::intToString(eggId) + NEWLINE;
}

String Protocole::eggHatched(int eggId)
{ 
	return verb(PROTOCOL_EHT) + " #" + mzu::intToString(eggId) + NEWLINE;
}

String Protocole::eggDeath(int eggId)
{ 
	return verb(PROTOCOL_EDI) + " #" + mzu::intToString(eggId) + NEWLINE;
}

String Protocole::gameEnd(const String &teamName)
{
	return verb(PROTOCOL_SEG) + " " + teamName + NEWLINE;
}

String Protocole::unknownCommand()
{
	return verb(PROTOCOL_SUC) + NEWLINE;
}

String Protocole::badParameters()
{
	return verb(PROTOCOL_SBP) + NEWLINE;
}

String Protocole::incantationStart(const s_incantation_context &context)
{
	String result = verb(PROTOCOL_PIC) + " " + mzu::intToString(context.x) + " " + mzu::intToString(context.y) + " " + mzu::intToString(context.level);
	for (size_t i = 0; i < context.playerIds.size(); i++)
		result += " #" + mzu::intToString(context.playerIds[i]);
	return result + NEWLINE;
}

String Protocole::incantationEnd(int x, int y, bool success)
{
	return verb(PROTOCOL_PIE) + " " + mzu::intToString(x) + " " + mzu::intToString(y) + " " + mzu::intToString(success ? 1 : 0) + NEWLINE;
}

String Protocole::serverMessage(const String &message)
{
	String clean = message;
	std::replace(clean.begin(), clean.end(), '\n', ' ');
	std::replace(clean.begin(), clean.end(), '\r', ' ');
	return verb(PROTOCOL_SMG) + " " + clean + NEWLINE;
}
