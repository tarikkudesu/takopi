#include "Game.hpp"
#include <cstdlib>

Game::Game() : __timeUnit(100), __clientsPerTeam(1), __nextPlayerId(0), __nextEggId(0)
{
	mzu::bzero(&__startTime, sizeof(__startTime));
}

Game::Game(const Game &copy) : __timeUnit(0), __clientsPerTeam(0), __nextPlayerId(0), __nextEggId(0)
{
	(void)copy;
}

Game &Game::operator=(const Game &assign)
{
	(void)assign;
	return *this;
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

void Game::init(int width, int height, const t_svec &teams, int timeUnit, int clientsPerTeam)
{
	__teams = teams;
	__timeUnit = timeUnit;
	__clientsPerTeam = clientsPerTeam;
	for (size_t i = 0; i < teams.size(); i++)
		__teamSlots[static_cast<int>(i)] = clientsPerTeam;
	srand(static_cast<unsigned int>(time(NULL)));
	__world.init(width, height);
	__world.populateResources();
	gettimeofday(&__startTime, NULL);
	__nextPlayerId = 0;
	__nextEggId = 0;
	mzu::info("game initialized: " + mzu::intToString(width) + "x" + mzu::intToString(height)
			  + ", " + mzu::intToString(static_cast<int>(teams.size())) + " teams, t=" + mzu::intToString(timeUnit)
			  + ", c=" + mzu::intToString(clientsPerTeam));
}

long Game::getCurrentTick() const
{
	struct timeval now;
	gettimeofday(&now, NULL);
	long elapsedUs = (now.tv_sec - __startTime.tv_sec) * 1000000L
				   + (now.tv_usec - __startTime.tv_usec);
	return elapsedUs * __timeUnit / 1000000L;
}

/*************************************************************************
 *                           GAME TICK                                   *
 *************************************************************************/

void Game::tick()
{
	long currentTick = getCurrentTick();
	processFood(currentTick);
	processEggs(currentTick);
	processCommands(currentTick);
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
	for (size_t i = 0; i < __eggs.size(); i++)
	{
		if (!__eggs[i]->isHatched() && __eggs[i]->tryHatch(currentTick))
		{
			__teamSlots[__eggs[i]->getTeamIndex()]++;
			mzu::info("egg " + mzu::intToString(__eggs[i]->getId()) + " hatched");
		}
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
	e_command type = CommandParser::parseCommandType(rawCmd);
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
		case CMD_AVANCE:		return executeAvance(cmd.getPlayerId());
		case CMD_DROITE:		return executeDroite(cmd.getPlayerId());
		case CMD_GAUCHE:		return executeGauche(cmd.getPlayerId());
		case CMD_VOIR:			return executeVoir(cmd.getPlayerId());
		case CMD_INVENTAIRE:	return executeInventaire(cmd.getPlayerId());
		case CMD_PREND:			return executePrendre(cmd.getPlayerId(), cmd.getArgument());
		case CMD_POSE:			return executePoser(cmd.getPlayerId(), cmd.getArgument());
		case CMD_EXPULSE:		return executeExpulse(cmd.getPlayerId());
		case CMD_BROADCAST:		return executeBroadcast(cmd.getPlayerId(), cmd.getArgument());
		case CMD_INCANTATION:	return executeIncantation(cmd.getPlayerId());
		case CMD_FORK:			return executeFork(cmd.getPlayerId());
		case CMD_CONNECT_NBR:	return executeConnectNbr(cmd.getPlayerId());
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
	return __world.buildVisionString(*player) + "\n";
}

String Game::executeInventaire(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	return player->inventoryString() + "\n";
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
		addNotification(otherId, "deplacement " + mzu::intToString(dir) + "\n");
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
		addNotification(it->first, "message " + mzu::intToString(dir) + "," + text + "\n");
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
				addNotification(pids[i], "niveau actuel : " + mzu::intToString(newLevel) + "\n");
		}
	}
	return "niveau actuel : " + mzu::intToString(newLevel) + "\n";
}

String Game::executeFork(int playerId)
{
	Player *player = getPlayer(playerId);
	if (!player)
		return "ko\n";
	long currentTick = getCurrentTick();
	Egg *egg = new Egg(__nextEggId, player->getX(), player->getY(),
					   player->getTeamIndex(), playerId,
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
	return mzu::intToString(remaining) + "\n";
}

/*************************************************************************
 *                          HANDSHAKE                                    *
 *************************************************************************/

String Game::handleHandshake(const String &teamName, int &outPlayerId)
{
	int teamIndex = getTeamIndex(teamName);
	if (teamIndex < 0)
	{
		outPlayerId = -1;
		return "ko\n";
	}
	if (__teamSlots[teamIndex] <= 0)
	{
		outPlayerId = -1;
		return "ko\n";
	}
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

	String response = mzu::intToString(__teamSlots[teamIndex]) + "\n";
	response += mzu::intToString(__world.getWidth()) + " " + mzu::intToString(__world.getHeight()) + "\n";
	return response;
}

/*************************************************************************
 *                        COMMAND QUEUE                                  *
 *************************************************************************/

void Game::enqueueCommand(int playerId, const String &rawCommand)
{
	__pendingCommands[playerId].push(rawCommand);
}

bool Game::canAcceptCommand(int playerId) const
{
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

int Game::getRemainingSlots(int teamIndex) const
{
	std::map<int, int>::const_iterator it = __teamSlots.find(teamIndex);
	if (it != __teamSlots.end())
		return it->second;
	return 0;
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
	addNotification(playerId, "mort\n");
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
