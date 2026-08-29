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

Core::Core()
{
}
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
	for (std::vector<int>::iterator it = tmpMapV.begin(); it != tmpMapV.end(); it++)
		Core::removeServer(*it);
	for (std::vector<int>::iterator it = tmpMapC.begin(); it != tmpMapC.end(); it++)
		Core::removeConnection(*it);
	tmpMapV.clear();
	tmpMapC.clear();
	Core::__servers.clear();
	Core::__connections.clear();
	for (int i = 3; i < 1024; i++)
		close(i);
}
void Core::removeConnection(int sd)
{
	mzu::info("removing connection " + mzu::intToString(sd));
	t_Connections::iterator it = Core::__connections.find(sd);
	if (it != Core::__connections.end())
	{
		Connection *instance = it->second;
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
	for (t_Server::iterator it = Core::__servers.begin(); it != Core::__servers.end(); it++)
	{
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
	else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != ECONNABORTED)
	{
		removeServer(sd);
	}
}

void Core::proccessSelectEvent(int sd, fd_set &readSet, fd_set &writeSet, int &retV)
{
	if (FD_ISSET(sd, &readSet))
	{
		if (isServerSocket(sd))
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
			it->second->processData();
		}
		catch (std::exception &e)
		{
			mzu::warn("connection " + mzu::intToString(it->second->getConnectionSocket()) + ": " + e.what());
			closeConnection.push_back(it->second->getConnectionSocket());
		}
	}

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
				if (connectionGame && connectionGame->getPlayerId() == pid)
				{
					connectionGame->pushOutput(msg);
					if (msg == "mort\n")
						connectionGame->setState(PLAYER_DEAD);
					break;
				}
			}
		}
		game->clearNotifications();
	}

	for (std::vector<int>::iterator it = closeConnection.begin(); it != closeConnection.end(); it++)
	{
		Core::removeConnection(*it);
	}
}
void Core::mainLoop()
{
	int retV = 0;
	fd_set readSet, writeSet;

	if (Core::__servers.empty())
		throw std::runtime_error("configuration does not identify any functional server");

	for (t_Server::iterator it = __servers.begin(); it != __servers.end(); it++)
	{
		if (it->second->getType() == GAME)
		{
			ServerGame *game = static_cast<ServerGame *>(it->second);
			game->initGame();
		}
	}

	Core::up = true;
	try
	{
		while (Core::up)
		{
			int maxFd = Core::buildSets(readSet, writeSet);
			struct timeval tv;
			tv.tv_sec = SELECT_TIMEOUT / 1000;
			tv.tv_usec = (SELECT_TIMEOUT % 1000) * 1000;
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
	catch (std::exception &e)
	{
		mzu::terr(e.what());
	}
}
