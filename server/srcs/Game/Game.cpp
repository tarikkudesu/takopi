#include "Game.hpp"
#include "CommandParser.hpp"
#include "Elevation.hpp"
#include <cstdlib>

Game::Game() :  __state(GAME_RUNNING),
				__timeUnit(100),
				__nextEggId(0),
				__nextPlayerId(0),
				__tickOffset(0)
{
	mzu::bzero(&__startTime, sizeof(__startTime));
}

Game::~Game()
{
	for (std::map<int, Player *>::iterator it = __players.begin(); it != __players.end(); ++it)
		delete it->second;
	__players.clear();
	for (size_t i = 0; i < __eggs.size(); i++)
		delete __eggs[i];
	__eggs.clear();
}

/*************************************************************************
 *                         INITIALIZATION                                *
 *************************************************************************/

void Game::init(int width, int height, const t_svec &teams, int timeUnit)
{
	for (std::map<int, Player *>::iterator it = __players.begin(); it != __players.end(); it++)
		delete it->second;
	for (size_t i = 0; i < __eggs.size(); i++)
		delete __eggs[i];
	__eggs.clear();
	__teams.clear();
	__players.clear();
	__teamSlots.clear();
	__notifications.clear();
	__activeCommands.clear();
	__pendingCommands.clear();
	__teams = teams;
	__tickOffset = 0;
	__timeUnit = timeUnit;
	__state = GAME_RUNNING;
	for (size_t i = 0; i < teams.size(); i++)
		__teamSlots[static_cast<int>(i)] = CLIENTS_PER_TEAM;
	srand(static_cast<unsigned int>(time(NULL)));
	__world.init(width, height);
	__world.populateResources();
	gettimeofday(&__startTime, NULL);
	__nextPlayerId = 0;
	__nextEggId = 0;
	mzu::info("game initialized: " + mzu::intToString(width) + "x" + mzu::intToString(height)
			  + ", " + mzu::intToString(static_cast<int>(teams.size())) + " teams, t=" + mzu::intToString(timeUnit));
}

long Game::getCurrentTick() const
{
	struct timeval now;
	gettimeofday(&now, NULL);
	long elapsedUs = (now.tv_sec - __startTime.tv_sec) * 1000000L
				   + (now.tv_usec - __startTime.tv_usec);
	return static_cast<long>(__tickOffset + elapsedUs * static_cast<double>(__timeUnit) / 1000000.0);
}

/*************************************************************************
 *                             ADMINISTRATION                            *
 *************************************************************************/

void Game::resizeMap(int width, int height)
{
	__world.resize(width, height);
	for (std::map<int, Player *>::iterator it = __players.begin(); it != __players.end(); it++)
		it->second->setPosition(__world.wrapX(it->second->getX()), __world.wrapY(it->second->getY()));
	for (size_t i = 0; i < __eggs.size(); i++)
		__eggs[i]->setPosition(__world.wrapX(__eggs[i]->getX()), __world.wrapY(__eggs[i]->getY()));
}

void Game::setTimeUnit(int timeUnit)
{
	if (timeUnit < 1 || timeUnit > 10000)
		throw std::runtime_error("time unit must be between 1 and 10000");
	struct timeval now;
	gettimeofday(&now, NULL);
	double elapsedUs = (now.tv_sec - __startTime.tv_sec) * 1000000.0 + now.tv_usec - __startTime.tv_usec;
	__tickOffset += elapsedUs * __timeUnit / 1000000.0;
	__startTime = now;
	__timeUnit = timeUnit;
}

/*************************************************************************
 *                           GAME TICK                                   *
 *************************************************************************/

void Game::tick()
{
	if (__state != GAME_RUNNING)
		return;
	long currentTick = getCurrentTick();
	processFood(currentTick);
	processEggs(currentTick);
	processCommands(currentTick);
	checkVictory();
	__world.display();
}

void Game::processFood(long currentTick)
{
	std::vector<int> deadPlayers;
	for (std::map<int, Player *>::iterator it = __players.begin(); it != __players.end(); ++it)
	{
		Player *player = it->second;
		if (!player->isAlive())
			continue;
		while (currentTick >= player->getNextFoodTick())
		{
			if (player->getInventory(NOURRITURE) > 0)
			{
				player->removeFromInventory(NOURRITURE);
				player->setNextFoodTick(player->getNextFoodTick() + FOOD_LIFE_UNITS);
			}
			else
			{
				deadPlayers.push_back(player->getId());
				break;
			}
		}
	}
	for (size_t i = 0; i < deadPlayers.size(); i++)
		killPlayer(deadPlayers[i]);
}

void Game::processEggs(long currentTick)
{
	for (std::vector<Egg *>::iterator it = __eggs.begin(); it != __eggs.end(); )
	{
		Egg *egg = *it;
		if (egg->tryHatch(currentTick))
		{
			__teamSlots[egg->getTeamIndex()]++;
			mzu::info("egg " + mzu::intToString(egg->getId()) + " hatched");
			delete egg;
			it = __eggs.erase(it);
		}
		else
			it++;
	}
}

void Game::processCommands(long currentTick)
{
	std::vector<int> completedPlayers;
	for (std::map<int, Command>::iterator it = __activeCommands.begin(); it != __activeCommands.end(); ++it)
	{
		if (currentTick >= it->second.getExecutionTime())
		{
			int pid = it->first;
			Player *player = getPlayer(pid);
			if (player && player->isAlive())
			{
				String response = executeCommand(it->second);
				addNotification(pid, response);
				checkVictory();
				if (__state != GAME_RUNNING)
					return;
			}
			completedPlayers.push_back(pid);
		}
	}
	for (size_t i = 0; i < completedPlayers.size(); i++)
		__activeCommands.erase(completedPlayers[i]);

	for (std::map<int, Player *>::iterator it = __players.begin(); it != __players.end(); ++it)
	{
		int pid = it->first;
		if (!it->second->isAlive())
			continue;
		if (__activeCommands.find(pid) != __activeCommands.end())
			continue;

		std::map<int, std::queue<String> >::iterator pit = __pendingCommands.find(pid);
		if (pit == __pendingCommands.end() || pit->second.empty())
			continue;

		String rawCmd = pit->second.front();
		pit->second.pop();
		if (pit->second.empty())
			__pendingCommands.erase(pit);

		activateCommand(pid, rawCmd, currentTick);
	}
}

void Game::activateCommand(int playerId, const String &rawCmd, long currentTick)
{
	t_command type = CommandParser::parseCommandType(rawCmd);
	String arg = CommandParser::parseArgument(rawCmd);

	if (type == CMD_UNKNOWN)
	{
		addNotification(playerId, "ko\n");
		return;
	}
	if (type == CMD_INCANTATION)
	{
		Player *player = getPlayer(playerId);
		if (!player)
			return;
		Tile &tile = __world.tileAt(player->getX(), player->getY());
		int sameLvl = countSameLevelPlayers(playerId);
		if (!Elevation::canElevate(player->getLevel(), tile, sameLvl))
		{
			addNotification(playerId, "ko\n");
			return;
		}
		addNotification(playerId, "elevation en cours\n");
	}
	int duration = Command::durationForCommand(type);
	Command cmd(type, arg, currentTick + duration, playerId);
	__activeCommands[playerId] = cmd;
}

/*************************************************************************
 *                       COMMAND EXECUTION                               *
 *************************************************************************/

String Game::executeCommand(const Command &cmd)
{
	switch (cmd.getType())
	{
		case CMD_VOIR:			return executeVoir(cmd.getPlayerId());
		case CMD_FORK:			return executeFork(cmd.getPlayerId());
		case CMD_AVANCE:		return executeAvance(cmd.getPlayerId());
		case CMD_DROITE:		return executeDroite(cmd.getPlayerId());
		case CMD_GAUCHE:		return executeGauche(cmd.getPlayerId());
		case CMD_EXPULSE:		return executeExpulse(cmd.getPlayerId());
		case CMD_INVENTAIRE:	return executeInventaire(cmd.getPlayerId());
		case CMD_CONNECT_NBR:	return executeConnectNbr(cmd.getPlayerId());
		case CMD_INCANTATION:	return executeIncantation(cmd.getPlayerId());
		case CMD_BROADCAST:		return executeBroadcast(cmd.getPlayerId(), cmd.getArgument());
		case CMD_PREND:			return executePrendre(cmd.getPlayerId(), cmd.getArgument());
		case CMD_POSE:			return executePoser(cmd.getPlayerId(), cmd.getArgument());
		default:				return "ko\n";
	}
}

String Game::executeAvance(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	__world.tileAt(player->getX(), player->getY()).removePlayer(playerId);
	player->moveForward(__world.getWidth(), __world.getHeight());
	__world.tileAt(player->getX(), player->getY()).addPlayer(playerId);
	return "ok\n";
}

String Game::executeDroite(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	player->turnRight();
	return "ok\n";
}

String Game::executeGauche(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	player->turnLeft();
	return "ok\n";
}

String Game::executeVoir(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	return __world.buildVisionString(*player) + NEWLINE;
}

String Game::executeInventaire(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	return player->inventoryString() + NEWLINE;
}

String Game::executePrendre(int playerId, const String &object)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	e_resource res = CommandParser::resourceFromName(object);
	if (res >= RESOURCE_COUNT)
		return "ko\n";
	Tile &tile = __world.tileAt(player->getX(), player->getY());
	if (!tile.hasResource(res))
		return "ko\n";
	tile.removeResource(res);
	player->addToInventory(res, 1);
	return "ok\n";
}

String Game::executePoser(int playerId, const String &object)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	e_resource res = CommandParser::resourceFromName(object);
	if (res >= RESOURCE_COUNT)
		return "ko\n";
	if (!player->removeFromInventory(res))
		return "ko\n";
	__world.tileAt(player->getX(), player->getY()).addResource(res, 1);
	return "ok\n";
}

String Game::executeExpulse(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	Tile &tile = __world.tileAt(player->getX(), player->getY());
	std::vector<int> onTile = tile.getPlayerIds();
	bool kicked = false;
	for (size_t i = 0; i < onTile.size(); i++)
	{
		int otherId = onTile[i];
		if (otherId == playerId)
			continue;
		Player *other = getPlayer(otherId);
		if (!other || !other->isAlive())
			continue;
		tile.removePlayer(otherId);
		other->moveInDirection(player->getDirection(), __world.getWidth(), __world.getHeight());
		__world.tileAt(other->getX(), other->getY()).addPlayer(otherId);
		int dir = __world.broadcastDirection(player->getX(), player->getY(),
											 other->getX(), other->getY(),
											 other->getDirection());
		addNotification(otherId, "deplacement " + mzu::intToString(dir) + NEWLINE);
		kicked = true;
	}
	return kicked ? "ok\n" : "ko\n";
}

String Game::executeBroadcast(int playerId, const String &text)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	for (std::map<int, Player *>::iterator it = __players.begin(); it != __players.end(); ++it)
	{
		if (it->first == playerId || !it->second->isAlive())
			continue;
		int dir = __world.broadcastDirection(player->getX(), player->getY(),
											 it->second->getX(), it->second->getY(),
											 it->second->getDirection());
		addNotification(it->first, "message " + mzu::intToString(dir) + "," + text + NEWLINE);
	}
	return "ok\n";
}

String Game::executeIncantation(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	Tile &tile = __world.tileAt(player->getX(), player->getY());
	int sameLvl = countSameLevelPlayers(playerId);
	if (!Elevation::canElevate(player->getLevel(), tile, sameLvl))
		return "ko\n";
	int currentLvl = player->getLevel();
	Elevation::consumeStones(currentLvl, tile);
	int newLevel = currentLvl + 1;
	const std::vector<int> &pids = tile.getPlayerIds();
	for (size_t i = 0; i < pids.size(); i++)
	{
		Player *p = getPlayer(pids[i]);
		if (p && p->isAlive() && p->getLevel() == currentLvl)
		{
			p->setLevel(newLevel);
			if (pids[i] != playerId)
				addNotification(pids[i], "niveau actuel : " + mzu::intToString(newLevel) + NEWLINE);
		}
	}
	return "niveau actuel : " + mzu::intToString(newLevel) + NEWLINE;
}

String Game::executeFork(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	long currentTick = getCurrentTick();
	Egg *egg = new Egg(__nextEggId, player->getX(), player->getY(),
					   player->getTeamIndex(),
					   currentTick + EGG_HATCH_DURATION);
	__eggs.push_back(egg);
	__nextEggId++;
	return "ok\n";
}

String Game::executeConnectNbr(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	int remaining = __teamSlots[player->getTeamIndex()];
	return mzu::intToString(remaining) + NEWLINE;
}

/*************************************************************************
 *                          HANDSHAKE                                    *
 *************************************************************************/

String Game::handleHandshake(const String &teamName, int &outPlayerId)
{
	outPlayerId = -1;
	if (__state != GAME_RUNNING)
		return "ko\n";
	int teamIndex = getTeamIndex(teamName);
	if (teamIndex < 0)
		return "ko\n";
	if (__teamSlots[teamIndex] <= 0)
		return "ko\n";
	int x = rand() % __world.getWidth();
	int y = rand() % __world.getHeight();
	e_direction dir = static_cast<e_direction>(rand() % 4);
	long currentTick = getCurrentTick();

	Player *player = new Player(__nextPlayerId, x, y, teamIndex, currentTick);
	player->setDirection(dir);
	__players[__nextPlayerId] = player;
	__world.tileAt(x, y).addPlayer(__nextPlayerId);
	__teamSlots[teamIndex]--;
	outPlayerId = __nextPlayerId;
	__nextPlayerId++;

	String response = mzu::intToString(__teamSlots[teamIndex]) + NEWLINE;
	response += mzu::intToString(__world.getWidth()) + " " + mzu::intToString(__world.getHeight()) + NEWLINE;
	return response;
}

/*************************************************************************
 *                        COMMAND QUEUE                                  *
 *************************************************************************/

void Game::enqueueCommand(int playerId, const String &rawCommand)
{
	Player *player = getPlayer(playerId);
	if (__state == GAME_RUNNING && player && player->isAlive() && canAcceptCommand(playerId))
		__pendingCommands[playerId].push(rawCommand);
}

bool Game::canAcceptCommand(int playerId) const
{
	if (__state != GAME_RUNNING)
		return false;
	std::map<int, std::queue<String> >::const_iterator it = __pendingCommands.find(playerId);
	if (it == __pendingCommands.end())
		return true;
	int total = static_cast<int>(it->second.size());
	if (__activeCommands.find(playerId) != __activeCommands.end())
		total++;
	return total < MAX_PENDING_COMMANDS;
}

/*************************************************************************
 *                         NOTIFICATIONS                                 *
 *************************************************************************/

void Game::addNotification(int playerId, const String &msg)
{
	s_notification n;
	n.playerId = playerId;
	n.message = msg;
	__notifications.push_back(n);
}

const std::vector<s_notification> &Game::getNotifications() const
{
	return __notifications;
}

void Game::clearNotifications()
{
	__notifications.clear();
}

/*************************************************************************
 *                           HELPERS                                     *
 *************************************************************************/

Player *Game::getPlayer(int playerId)
{
	std::map<int, Player *>::iterator it = __players.find(playerId);
	if (it != __players.end())
		return it->second;
	return NULL;
}

int Game::getTeamIndex(const String &teamName) const
{
	for (size_t i = 0; i < __teams.size(); i++)
	{
		if (__teams[i] == teamName)
			return static_cast<int>(i);
	}
	return -1;
}

void Game::killPlayer(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player || !player->isAlive())
		return;
	player->setState(PLAYER_DEAD);
	__world.tileAt(player->getX(), player->getY()).removePlayer(playerId);
	__activeCommands.erase(playerId);
	__pendingCommands.erase(playerId);
	addNotification(playerId, PLAYER_DEATH_MESSAGE);
}

void Game::removePlayer(int playerId)
{
	std::map<int, Player *>::iterator player = __players.find(playerId);
	if (player == __players.end())
		return;
	if (player->second->isAlive())
		__world.tileAt(player->second->getX(), player->second->getY()).removePlayer(playerId);
	__activeCommands.erase(playerId);
	__pendingCommands.erase(playerId);
	for (std::vector<s_notification>::iterator it = __notifications.begin(); it != __notifications.end(); )
	{
		if (it->playerId == playerId)
			it = __notifications.erase(it);
		else
			it++;
	}
	delete player->second;
	__players.erase(player);
}

int Game::countSameLevelPlayers(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return 0;
	Tile &tile = __world.tileAt(player->getX(), player->getY());
	const std::vector<int> &pids = tile.getPlayerIds();
	int count = 0;
	for (size_t i = 0; i < pids.size(); i++)
	{
		Player *p = getPlayer(pids[i]);
		if (p && p->isAlive() && p->getLevel() == player->getLevel())
			count++;
	}
	return count;
}

/*************************************************************************
 *                          ADMIN COMMANDS                               *
 *************************************************************************/

int Game::getTimeUnit() const
{
	return __timeUnit;
}

String Game::executeAdminCommand(t_command type, const t_svec &args)
{
	if (__state != GAME_RUNNING)
		return ADMIN_ERR "Game is ending.\n";
	int values[3] = {0, 0, 0};

	for (size_t i = 1; i < args.size(); i++)
	{
		if (args.at(i).find_first_not_of("0123456789") != String::npos)
			return "ERR arguments must be unsigned integers\n";
		std::istringstream number(args.at(i));
		if (!(number >> values[i - 1]))
			return "ERR integer out of range\n";
	}
	try
	{
		switch (type)
		{
			case CMD_ADMIN_RESIZE:	resizeMap(values[0], values[1]); break;
			case CMD_ADMIN_RETIME:	setTimeUnit(values[0]); break;
			default:				return "ERR invalid game command\n";
		}
	}
	catch (const std::exception &e)
	{
		mzu::debug(e.what());
		return String("ERR ") + e.what() + NEWLINE;
	}
	return "OK " + args.at(0) + NEWLINE;
}

/*************************************************************************
 *                          GAME COMPLETION                              *
 *************************************************************************/

t_game_state Game::getState() const
{
	return __state;
}

void Game::checkVictory()
{
	if (__state != GAME_RUNNING)
		return;
	for (size_t team = 0; team < __teams.size(); team++)
	{
		int count = 0;
		bool allMaxLevel = true;
		for (std::map<int, Player *>::iterator it = __players.begin(); it != __players.end(); it++)
		{
			Player *player = it->second;
			if (!player->isAlive() || player->getTeamIndex() != static_cast<int>(team))
				continue;
			count++;
			if (player->getLevel() != WIN_LEVEL)
				allMaxLevel = false;
		}
		if (count < CLIENTS_PER_TEAM || !allMaxLevel)
			continue;
		__state = GAME_ENDING;
		mzu::info("game ended: team " + __teams[team] + " won");
		for (std::map<int, Player *>::iterator it = __players.begin(); it != __players.end(); it++)
			killPlayer(it->first);
		__activeCommands.clear();
		__pendingCommands.clear();
		return;
	}
}
