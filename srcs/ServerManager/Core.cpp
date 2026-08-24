#include "Core.hpp"

bool Core::up = false;
int Core::__allocN = 0;
int Core::__sockNum = 0;
t_Server Core::__servers;
t_events Core::__sockets;
t_Connections Core::__connections;
struct pollfd *Core::__events = NULL;

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

void Core::clear()
{
	wsu::debug("clearing data");
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
	Core::__sockNum = 0;
	Core::__servers.clear();
	Core::__sockets.clear();
	Core::__connections.clear();
    for (int i = 3; i < 1024; i++)
        close(i);
}
void Core::removeConnection(int sd)
{
	wsu::info("removing connection " + wsu::intToString(sd));
	t_Connections::iterator it = Core::__connections.find(sd);
	if (it != Core::__connections.end())
	{
		Connection *instance = it->second;
		Core::__connections.erase(it);
		delete instance;
		removeSocket(sd);
	}
}
void Core::addConnection(Connection *connection)
{
	wsu::info("creating connection " + wsu::intToString(connection->getConnectionSocket()));
	if (Core::__sockNum >= MAX_EVENTS)
		throw std::runtime_error("critical server overload, could not accept new client connection");
	Core::__connections[connection->getConnectionSocket()] = connection;
	addSocket(connection->getConnectionSocket(), CONNECTION);
}
void Core::removeServer(int sd)
{
	wsu::info("removing server " + wsu::intToString(sd));
	t_Server::iterator it = Core::__servers.find(sd);
	if (it != Core::__servers.end())
	{
		Server *instance = it->second;
		Core::__servers.erase(it);
		delete instance;
		removeSocket(sd);
	}
}
void Core::addServer(Server *server)
{
	wsu::info("creating server " + wsu::intToString(server->getServerSocket()));
	if (Core::__sockNum >= MAX_EVENTS)
		throw std::runtime_error("critical server overload, " + server->getServerHost() + ":" + wsu::intToString(server->getServerPort()) + " non functional");
	Core::__servers[server->getServerSocket()] = server;
	addSocket(server->getServerSocket(), SERVER);
}
void Core::removeSocket(int sd)
{
	for (t_events::iterator it = __sockets.begin(); it != __sockets.end(); it++)
	{
		if (sd == it->fd)
		{
			Core::__sockets.erase(it);
			Core::__sockNum--;
			close(sd);
			return;
		}
	}
}
void Core::addSocket(int sd, t_endian endian)
{
	struct pollfd sockStruct;
	sockStruct.fd = sd;
	if (endian == SERVER)
		sockStruct.events = POLLIN;
	else if (endian == CONNECTION)
		sockStruct.events = POLLIN | POLLOUT | POLLHUP;
	if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl syscall, failed to make a non blocking socket");
	Core::__sockets.push_back(sockStruct);
	Core::__sockNum++;
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
		wsu::running((*it).second->getServerHost() + ":" + wsu::intToString((*it).second->getServerPort()));
	}
}
void Core::checkConflicts()
{
	for (t_Server::iterator it = Core::__servers.begin(); it != Core::__servers.end(); it++)
	{
		for (t_Server::iterator iter = Core::__servers.begin(); iter != it && iter != Core::__servers.end(); iter++)
		{
			if (it->second->getServerPort() == iter->second->getServerPort())
			{
				const t_svec &serverNames = it->second->__serverNames;
				for (t_svec::const_iterator name = serverNames.begin(); name != serverNames.end(); name++)
				{
					if (iter->second->amITheServerYouAreLookingFor(*name))
						wsu::warn("conflicting server name \"" + *name + "\" on " + it->second->serverIdentity() + ", ignored");
				}
			}
		}
	}
}

/************************************************************************
 *							  SERVER CONTROL							*
 ************************************************************************/

void Core::writeDataToSocket(int sd)
{
	t_Connections::iterator iter = Core::__connections.find(sd);
	if (iter == Core::__connections.end())
		return;
	if (wsu::__criticalOverLoad == true && iter->second->__responseQueue.empty())
		return Core::removeConnection(sd);
	if (iter->second->__responseQueue.empty())
		return;

	ssize_t bytesWritten = send(sd,
								iter->second->__responseQueue.front().getBuff(),
								iter->second->__responseQueue.front().length(), 0);
	iter->second->__responseQueue.pop();
	if (bytesWritten > 0)
	{
		wsu::info("response sent");
		if (iter->second->close())
			removeConnection(sd);
	}
	else
	{
        removeConnection(sd);
	}
}
void Core::readDataFromSocket(int sd)
{
	char buff[READ_SIZE + 1];
	t_Connections::iterator iter = Core::__connections.find(sd);
	if (iter == Core::__connections.end())
		return;

	ssize_t bytesRead = recv(sd, buff, READ_SIZE, 0);
	if (bytesRead == 0)
		removeConnection(sd);
	else if (bytesRead > 0)
	{
		buff[bytesRead] = '\0';
		iter->second->addData(BasicString(buff, bytesRead));
	}
	else
	{
        removeConnection(sd);
	}
}

void Core::acceptNewConnection(int sd)
{
	int newSock;

	if (Core::__sockNum >= MAX_EVENTS)
		wsu::__criticalOverLoad = true;
	if (wsu::__criticalOverLoad == true)
		return;
	newSock = accept(sd, NULL, NULL);
	if (newSock >= 0)
	{
		Connection *newConnection = new Connection(sd);
		try
		{
			newConnection->setSocket(newSock);
			newConnection->setServers(Core::__servers);
			Core::addConnection(newConnection);
		}
		catch (std::exception &e)
		{
			delete newConnection;
			wsu::error(e.what());
		}
	}
	else
	{
		removeServer(sd);
	}
}
void Core::proccessPollEvent(int sd, int &retV)
{
	struct pollfd &sockStruct = Core::__events[sd];

	if (sockStruct.revents & POLLIN)
	{
		if (isServerSocket(sockStruct.fd))
		{
			if (Core::__sockets.size() >= MAX_EVENTS)
				wsu::__criticalOverLoad = true;
			else
			{
				acceptNewConnection(sockStruct.fd);
				retV--;
			}
		}
		else
		{
			readDataFromSocket(sockStruct.fd);
			retV--;
		}
	}
	else if (sockStruct.revents & POLLOUT)
	{
		writeDataToSocket(sockStruct.fd);
		retV--;
	}
	else if (sockStruct.revents & POLLHUP)
	{
		removeConnection(sockStruct.fd);
		retV--;
	}
	else if (wsu::__criticalOverLoad == true)
	{
		wsu::fatal("critcal server overload");
		if (!Core::isServerSocket(sockStruct.fd))
		{
			removeConnection(sockStruct.fd);
		}
	}
	if (Core::__servers.size() == Core::__sockets.size())
		wsu::__criticalOverLoad = false;
}

/***************************************************************************************
 *										MAIN LOOP									   *
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
			closeConnection.push_back(it->second->getConnectionSocket());
		}
	}
	for (std::vector<int>::iterator it = closeConnection.begin(); it != closeConnection.end(); it++)
	{
		Core::removeConnection(*it);
	}
}
void Core::mainLoop()
{
	int retV = 0;

	if (Core::__sockNum < MAX_EVENTS)
		Core::up = true;
	if (Core::__sockNum == 0)
		throw std::runtime_error("config file does not identify any functional server");
	try
	{
		while (Core::up)
		{
			Core::__allocN = Core::__sockNum;
			Core::__events = wsu::data(Core::__sockets);
			retV = poll(Core::__events, Core::__allocN, POLL_TIMEOUT);
			if (retV != 0 && retV != -1)
			{
				try {
					for (int sd = 0; sd < Core::__allocN && retV; sd++)
					{
						if (wsu::__criticalOverLoad == true)
							retV = Core::__allocN;
						Core::proccessPollEvent(sd, retV);
					}
					Core::mainProcess();

				} catch(wsu::Exit &e)
				{
					Core::up = false;
				}
			}
			delete[] Core::__events;
			Core::__events = NULL;
		}
	}
	catch (std::exception &e)
	{
		wsu::terr(e.what());
	}
}
