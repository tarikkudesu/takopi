#include "Core.hpp"
#include "ServerAdmin.hpp"
#include "ServerGame.hpp"
#include "../Game/Game.hpp"
#include "../Game/ConnectionAdmin.hpp"
#include "../Game/ConnectionGame.hpp"
#include "../Game/ConnectionGui.hpp"

bool Core::up = false;
t_Server Core::__servers;
t_Connections Core::__connections;
bool Core::__consoleDiscarding = false;
bool Core::__consoleOpen = true;
String Core::__consoleBuffer;

Core::Core() { }
Core::Core(const Core &copy)
{
	(void)copy;
}
Core &Core::operator=(const Core &assign)
{
	(void)assign;
	return *this;
}
Core::~Core()
{
	clear();
}

/****************************************************************************
 *								 MINI METHODS								*
 ****************************************************************************/

int Core::currentLoad()
{
	return Core::__servers.size() + Core::__connections.size();
}

void Core::clear()
{
	mzu::debug("clearing data");
	std::vector<int> tmpMapV, tmpMapC;
	for (t_Server::iterator it = Core::__servers.begin(); it != Core::__servers.end(); it++)
		tmpMapV.push_back(it->second->getServerSocket());
	for (t_Connections::iterator it = Core::__connections.begin(); it != Core::__connections.end(); it++)
		tmpMapC.push_back(it->second->getConnectionSocket());
	for (std::vector<int>::iterator it = tmpMapC.begin(); it != tmpMapC.end(); it++)
		Core::removeConnection(*it);
	for (std::vector<int>::iterator it = tmpMapV.begin(); it != tmpMapV.end(); it++)
		Core::removeServer(*it);
	tmpMapV.clear();
	tmpMapC.clear();
	Core::__servers.clear();
	Core::__connections.clear();
	Core::up = false;
}
void Core::removeConnection(int sd)
{
	mzu::info("removing connection " + mzu::intToString(sd));
	t_Connections::iterator it = Core::__connections.find(sd);
	if (it != Core::__connections.end())
	{
		Connection *instance = it->second;
		ConnectionGame *connection = dynamic_cast<ConnectionGame *>(instance);
		if (connection)
		{
			Game *game = static_cast<ServerGame *>(connection->getServer())->getGame();
			if (game)
				game->killPlayer(connection->getPlayerId());
		}
		Core::__connections.erase(it);
		delete instance;
		close(sd);
	}
}
void Core::addConnection(Connection *connection)
{
	int sd = connection->getConnectionSocket();

	mzu::info("creating connection " + mzu::intToString(sd));
	if (Core::currentLoad() >= MAX_EVENTS)
		throw std::runtime_error("critical server overload, could not accept new client connection");
	if (sd >= FD_SETSIZE)
		throw std::runtime_error("file descriptor out of select range, could not accept new client connection");
	if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl syscall, failed to make a non blocking socket");
	Core::__connections[sd] = connection;
}
void Core::removeServer(int sd)
{
	mzu::info("removing server " + mzu::intToString(sd));
	t_Server::iterator it = Core::__servers.find(sd);
	if (it != Core::__servers.end())
	{
		Server *instance = it->second;
		std::vector<int> connections;
		for (t_Connections::iterator ci = __connections.begin(); ci != __connections.end(); ci++)
		{
			if (ci->second->getServer() == instance)
				connections.push_back(ci->first);
		}
		for (size_t i = 0; i < connections.size(); i++)
			removeConnection(connections[i]);
		Core::__servers.erase(it);
		delete instance;
	}
}
void Core::addServer(Server *server)
{
	int sd = server->getServerSocket();

	mzu::info("creating server " + mzu::intToString(sd));
	if (Core::currentLoad() >= MAX_EVENTS)
		throw std::runtime_error("critical server overload, " + server->getServerHost() + ":" + mzu::intToString(server->getServerPort()) + " non functional");
	if (sd >= FD_SETSIZE)
		throw std::runtime_error("file descriptor out of select range, " + server->getServerHost() + ":" + mzu::intToString(server->getServerPort()) + " non functional");
	if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl syscall, failed to make a non blocking socket");
	Core::__servers[sd] = server;
}
bool Core::hasGameServer()
{
	for (t_Server::iterator it = Core::__servers.begin(); it != Core::__servers.end(); it++)
	{
		if (it->second->getType() == GAME)
			return true;
	}
	return false;
}
bool Core::isServerSocket(int sd)
{
	if (Core::__servers.find(sd) != Core::__servers.end())
		return true;
	return false;
}

/****************************************************************************
 *									METHODS 								*
 ****************************************************************************/

void Core::logServers()
{
	t_Server::iterator it = Core::__servers.begin();
	for (; it != Core::__servers.end(); it++)
	{
		String type("");
		switch ((*it).second->getType())
		{
			case GUI: type = "GUI"; break;
			case GAME: type = "GAME"; break;
			case ADMIN: type = "ADMIN"; break;
			default: break;
		}
		mzu::running("[" + type + "] " + (*it).second->getServerHost() + ":" + mzu::intToString((*it).second->getServerPort()));
	}
}

/************************************************************************
 *						  SERVER CONTROL								*
 ************************************************************************/

int Core::buildSets(fd_set &readSet, fd_set &writeSet)
{
	int maxFd = -1;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	if (Core::__consoleOpen)
	{
		FD_SET(STDIN_FILENO, &readSet);
		maxFd = STDIN_FILENO;
	}
	for (t_Server::iterator it = Core::__servers.begin(); it != Core::__servers.end(); it++)
	{
		if (it->second->getType() == GAME)
		{
			Game *game = static_cast<ServerGame *>(it->second)->getGame();
			if (game && game->getState() == GAME_ENDING)
				continue;
		}
		int sd = it->second->getServerSocket();
		FD_SET(sd, &readSet);
		if (sd > maxFd)
			maxFd = sd;
	}
	for (t_Connections::iterator it = Core::__connections.begin(); it != Core::__connections.end(); it++)
	{
		int sd = it->second->getConnectionSocket();
		if (it->second->getType() == ADMIN)
		{
			ConnectionAdmin *connection = dynamic_cast<ConnectionAdmin *>(it->second);
			if (connection->tlsNeedsRead())
				FD_SET(sd, &readSet);
			if (connection->tlsNeedsWrite())
				FD_SET(sd, &writeSet);
		}
		else if (it->second->getType() == GAME)
		{
			ConnectionGame *connection = static_cast<ConnectionGame *>(it->second);
			Game *game = static_cast<ServerGame *>(connection->getServer())->getGame();
			if (connection->getState() != PLAYER_DEAD && game && game->getState() == GAME_RUNNING)
				FD_SET(sd, &readSet);
			if (connection->hasPendingOutput())
				FD_SET(sd, &writeSet);
		}
		else
		{
			FD_SET(sd, &readSet);
			FD_SET(sd, &writeSet);
		}
		if (sd > maxFd)
			maxFd = sd;
	}
	return maxFd;
}

void Core::processConsoleCommand(const String &command)
{
	String input = command;

	if (!input.empty() && input.at(input.length() - 1) == '\r')
		input.erase(input.length() - 1);
	if (input.empty())
	{
		std::cout << ADMIN_PROMPT << std::flush;
		return;
	}
	mzu::running("[admin local] command received: " + input);
	std::cout << Core::executeAdminCommand(input) << ADMIN_PROMPT << std::flush;
}

void Core::processConsoleInput()
{
	char buff[READ_SIZE];
	ssize_t bytesRead = read(STDIN_FILENO, buff, sizeof(buff));

	if (bytesRead == 0)
	{
		Core::__consoleOpen = false;
		mzu::warn("local administration console closed");
		return;
	}
	if (bytesRead < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return;
		Core::__consoleOpen = false;
		mzu::error("local administration console failed");
		return;
	}
	for (ssize_t i = 0; i < bytesRead; i++)
	{
		if (buff[i] == '\n')
		{
			if (Core::__consoleDiscarding)
			{
				mzu::warn("local administration command rejected: command exceeds maximum allowed size");
				std::cout << "ERR command exceeds maximum allowed size\n" << ADMIN_PROMPT << std::flush;
			}
			else
				Core::processConsoleCommand(Core::__consoleBuffer);
			Core::__consoleBuffer.clear();
			Core::__consoleDiscarding = false;
		}
		else if (!Core::__consoleDiscarding)
		{
			Core::__consoleBuffer += buff[i];
			if (Core::__consoleBuffer.length() > MAX_MESSAGE_SIZE)
			{
				Core::__consoleBuffer.clear();
				Core::__consoleDiscarding = true;
			}
		}
	}
}

void Core::writeDataToSocket(int sd)
{
	t_Connections::iterator iter = Core::__connections.find(sd);
	if (iter == Core::__connections.end())
		return;

	if (mzu::__criticalOverLoad == true && !iter->second->hasPendingOutput())
		return Core::removeConnection(sd);

	if (!iter->second->writeSocket())
		removeConnection(sd);
}
void Core::readDataFromSocket(int sd)
{
	t_Connections::iterator iter = Core::__connections.find(sd);
	if (iter == Core::__connections.end())
		return;

	if (!iter->second->readSocket())
		removeConnection(sd);
}

void Core::acceptNewConnection(int sd)
{
	int newSock;

	if (mzu::__criticalOverLoad == true)
		return;

	newSock = accept(sd, NULL, NULL);
	if (newSock >= 0)
	{
		if (newSock >= FD_SETSIZE)
		{
			close(newSock);
			mzu::error("rejected incoming client: file descriptor out of select range");
			return;
		}
		Server *server = Core::__servers[sd];
		switch (server->getType())
		{
			case ADMIN:
			{
				ServerAdmin *admin = dynamic_cast<ServerAdmin *>(Core::__servers[sd]);
				if (!admin)
				{
					mzu::warn("ServerAdmin daynamic_cast went wrong, you shouldn't be seeing this warning, it's worth investigating");
					break;
				}
				ConnectionAdmin *connectionAdmin = new ConnectionAdmin(server);
				Connection *connection = connectionAdmin;
				try
				{
					connection->setSocket(newSock);
					connectionAdmin->setupTLS(admin->getTLSContext());
					Core::addConnection(connection);
				}
				catch (std::exception &e)
				{
					mzu::error(e.what());
					delete connectionAdmin;
					close(newSock);
				}
				break;
			}
			case GAME:
			{
				ConnectionGame *connectionGame = new ConnectionGame(server);
				Connection *connection = connectionGame;
				try
				{
					connection->setSocket(newSock);
					Core::addConnection(connection);
				}
				catch (std::exception &e)
				{
					mzu::error(e.what());
					delete connectionGame;
					close(newSock);
				}
				break;
			}
			case GUI:
			{
				ConnectionGui *connectionGui = new ConnectionGui(server);
				Connection *connection = connectionGui;
				try
				{
					connection->setSocket(newSock);
					Core::addConnection(connection);
				}
				catch (std::exception &e)
				{
					mzu::error(e.what());
					delete connectionGui;
					close(newSock);
				}
				break;
			}
			default: break;
		}
	}
	else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR && errno != ECONNABORTED)
	{
		throw std::runtime_error("accept syscall failed");
	}
}

void Core::proccessSelectEvent(int sd, fd_set &readSet, fd_set &writeSet, int &retV)
{
	if (FD_ISSET(sd, &readSet))
	{
		if (sd == STDIN_FILENO && Core::__consoleOpen)
		{
			Core::processConsoleInput();
			retV--;
		}
		else if (isServerSocket(sd))
		{
			if (Core::currentLoad() >= MAX_EVENTS)
				mzu::__criticalOverLoad = true;
			else
			{
				acceptNewConnection(sd);
				retV--;
			}
		}
		else
		{
			readDataFromSocket(sd);
			retV--;
		}
	}
	else if (FD_ISSET(sd, &writeSet))
	{
		writeDataToSocket(sd);
		retV--;
	}
	else if (mzu::__criticalOverLoad == true)
	{
		mzu::fatal("critcal server overload");
		if (!Core::isServerSocket(sd))
		{
			removeConnection(sd);
		}
	}
	if (Core::__servers.size() == Core::__connections.size())
		mzu::__criticalOverLoad = false;
}

/***************************************************************************************
 *									MAIN LOOP									   *
 ***************************************************************************************/

void Core::mainProcess()
{
	std::vector<int> closeConnection;

	for (t_Connections::iterator it = __connections.begin(); it != __connections.end(); it++)
	{
		try
		{
			ConnectionGame *connection = dynamic_cast<ConnectionGame *>(it->second);
			if (connection && connection->getState() == PLAYER_DEAD)
				continue;
			it->second->processData();
		}
		catch (std::exception &e)
		{
			mzu::warn("connection " + mzu::intToString(it->second->getConnectionSocket()) + ": " + e.what());
			closeConnection.push_back(it->second->getConnectionSocket());
		}
	}

	for (size_t i = 0; i < closeConnection.size(); i++)
		Core::removeConnection(closeConnection[i]);
	closeConnection.clear();

	for (t_Server::iterator it = __servers.begin(); it != __servers.end(); it++)
	{
		if (it->second->getType() != GAME)
			continue;
		ServerGame *gs = static_cast<ServerGame *>(it->second);
		Game *game = gs->getGame();
		if (!game)
			continue;

		game->tick();

		const std::vector<s_notification> &notifications = game->getNotifications();
		for (size_t n = 0; n < notifications.size(); n++)
		{
			int pid = notifications[n].playerId;
			const String &msg = notifications[n].message;
			for (t_Connections::iterator ci = __connections.begin(); ci != __connections.end(); ci++)
			{
				ConnectionGame *connectionGame = dynamic_cast<ConnectionGame *>(ci->second);
				if (connectionGame && connectionGame->getServer() == gs && connectionGame->getPlayerId() == pid)
				{
					connectionGame->pushOutput(msg);
					if (msg == PLAYER_DEATH_MESSAGE)
						connectionGame->setState(PLAYER_DEAD);
					break;
				}
			}
		}
		game->clearNotifications();
	}

	for (t_Connections::iterator it = __connections.begin(); it != __connections.end(); it++)
	{
		ConnectionGame *connection = dynamic_cast<ConnectionGame *>(it->second);
		if (!connection)
			continue;
		Game *game = static_cast<ServerGame *>(connection->getServer())->getGame();
		if (game && game->getState() == GAME_ENDING)
			connection->setState(PLAYER_DEAD);
		if (connection->getState() == PLAYER_DEAD && !connection->hasPendingOutput())
			closeConnection.push_back(it->first);
	}
	for (size_t i = 0; i < closeConnection.size(); i++)
		Core::removeConnection(closeConnection[i]);

	std::vector<int> closeServer;
	for (t_Server::iterator it = __servers.begin(); it != __servers.end(); it++)
	{
		if (it->second->getType() != GAME)
			continue;
		Game *game = static_cast<ServerGame *>(it->second)->getGame();
		if (!game || game->getState() != GAME_ENDING)
			continue;
		bool hasConnections = false;
		for (t_Connections::iterator ci = __connections.begin(); ci != __connections.end(); ci++)
		{
			if (ci->second->getServer() == it->second)
			{
				hasConnections = true;
				break;
			}
		}
		if (!hasConnections)
			closeServer.push_back(it->first);
	}
	for (size_t i = 0; i < closeServer.size(); i++)
		Core::removeServer(closeServer[i]);
	if (!Core::hasGameServer())
		Core::up = false;
}
void Core::mainLoop()
{
	int retV = 0;
	fd_set readSet, writeSet;

	for (t_Server::iterator it = __servers.begin(); it != __servers.end(); it++)
	{
		if (it->second->getType() == GAME)
		{
			ServerGame *game = static_cast<ServerGame *>(it->second);
			game->initGame();
		}
	}
	int consoleFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
	if (consoleFlags < 0 || fcntl(STDIN_FILENO, F_SETFL, consoleFlags | O_NONBLOCK) < 0)
	{
		Core::__consoleOpen = false;
		mzu::warn("local administration console unavailable");
	}
	else
		std::cout << ADMIN_PROMPT << std::flush;

	Core::up = true;
	try
	{
		while (Core::up && Core::hasGameServer())
		{
			int maxFd = Core::buildSets(readSet, writeSet);
			struct timeval tv;
			long timeoutUs = SELECT_TIMEOUT * 1000;
			for (t_Server::iterator it = __servers.begin(); it != __servers.end(); it++)
			{
				if (it->second->getType() != GAME)
					continue;
				Game *game = static_cast<ServerGame *>(it->second)->getGame();
				if (game && game->getState() == GAME_RUNNING)
					timeoutUs = std::min(timeoutUs, 1000000L / game->getTimeUnit());
			}
			tv.tv_sec = timeoutUs / 1000000;
			tv.tv_usec = timeoutUs % 1000000;
			retV = select(maxFd + 1, &readSet, &writeSet, NULL, &tv);
			if (retV < 0)
			{
				if (errno == EINTR)
					continue;
				throw std::runtime_error("select syscall failed");
			}
			if (retV == 0)
			{
				Core::mainProcess();
				continue;
			}
			try
			{
				for (int sd = 0; sd <= maxFd && retV > 0; sd++)
				{
					if (mzu::__criticalOverLoad == true)
						retV = maxFd + 1;
					Core::proccessSelectEvent(sd, readSet, writeSet, retV);
				}
				Core::mainProcess();
			}
			catch (mzu::Exit &e)
			{
				Core::up = false;
			}
		}
	}
	catch (...)
	{
		Core::up = false;
		if (consoleFlags >= 0)
			fcntl(STDIN_FILENO, F_SETFL, consoleFlags);
		throw;
	}
	Core::up = false;
	if (consoleFlags >= 0)
		fcntl(STDIN_FILENO, F_SETFL, consoleFlags);
}

/*************************************************************************
 *                             ADMINISTRATION                            *
 *************************************************************************/

String Core::handleGamesCommand()
{
    String result = "ID   HOST              PORT";
    for (t_Server::iterator it = Core::__servers.begin(); it != Core::__servers.end(); it++)
    {
        if (it->second->getType() != GAME)
            continue;
        result += NEWLINE + mzu::intToString(it->first) + " " + it->second->getServerHost() + " " + mzu::intToString(it->second->getServerPort()) + NEWLINE;
    }
    return result;
}

String Core::routeGameCommand(t_command type, const t_svec &args)
{
    int gameId;
    if (args.at(1).empty() || args.at(1).find_first_not_of("0123456789") != String::npos)
        return ADMIN_ERR "Invalid game ID.\n";
    std::istringstream value(args.at(1));
    if (!(value >> gameId))
        return ADMIN_ERR "Invalid game ID.\n";
    t_Server::iterator it = Core::__servers.find(gameId);
    if (it == Core::__servers.end() || it->second->getType() != GAME)
        return ADMIN_ERR "Unknown game ID: " + mzu::intToString(gameId) + ".\n";
    Game *game = static_cast<ServerGame *>(it->second)->getGame();
    if (!game)
        return ADMIN_ERR "Game " + mzu::intToString(gameId) + " is not initialized.\n";
    return game->executeAdminCommand(type, args);
}

String Core::executeAdminCommand(const String &command)
{
    std::istringstream input(command);
    t_svec args;
    String word;

    while (input >> word)
        args.push_back(word);
    if (args.empty())
        return "";

    t_command type = CommandParser::parseCommandType(command);
    switch (type)
    {
		case CMD_ADMIN_HELP:
			if (args.size() != 1)
				return ADMIN_ERR "Usage: help";
			return "Available commands:\n"
				"  games\n"
				"      List all running game servers.\n"
				"  resize <game-id> <width> <height>\n"
				"      Resize the map of a running game.\n"
				"  time <game-id> <value>\n"
				"      Change the time unit of a running game.\n";

        case CMD_ADMIN_GAMES:
            if (args.size() != 1)
                return ADMIN_ERR "Usage: games";
            return Core::handleGamesCommand();

        case CMD_ADMIN_RESIZE:
            if (args.size() != 4)
                return ADMIN_ERR "Usage: resize <game-id> <width> <height>\n";
            return Core::routeGameCommand(type, args);

        case CMD_ADMIN_RETIME:
            if (args.size() != 3)
                return ADMIN_ERR "Usage: time <game-id> <value>\n";
            return Core::routeGameCommand(type, args);

        default:
            return ADMIN_ERR "Unknown command. Type 'help' for available commands.\n";
    }
}
