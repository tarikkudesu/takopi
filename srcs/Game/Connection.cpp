#include "Connection.hpp"
#include "../ServerManager/GameServer.hpp"
#include "../Game/Game.hpp"

Connection::Connection(Server *server) : __sd(-1), __server(server)
{
	mzu::debug("Connection constructor");
}
Connection::Connection(const Connection &copy) : __sd(-1), __server(NULL)
{
	mzu::debug("Connection copy constructor");
	*this = copy;
}
Connection &Connection::operator=(const Connection &assign)
{
	mzu::debug("Connection copy assignement operator");
	if (this != &assign)
	{
		__sd = assign.__sd;
		__server = assign.__server;
		__client = assign.__client;
		__responseQueue = assign.__responseQueue;
	}
	return *this;
}
Connection::~Connection()
{
	mzu::debug("Connection destructor");
}

void Connection::setSocket(int sd)
{
	this->__sd = sd;
	if (__server && __server->getServerType() == "game")
	{
		__client.setConnectionType(CONNECTION_GAME);
		__client.setState(PLAYER_HANDSHAKE);
		__responseQueue.push(BasicString("BIENVENUE\n"));
	}
	else if (__server && __server->getServerType() == "admin")
	{
		__client.setConnectionType(CONNECTION_ADMIN);
	}
	else if (__server && __server->getServerType() == "gui")
	{
		__client.setConnectionType(CONNECTION_GUI);
	}
}

int Connection::getConnectionSocket()
{
	return __sd;
}

/*****************************************************************************
 *								MESSAGE FRAMING							 *
 *****************************************************************************/

void Connection::addData(const BasicString &input)
{
	this->__buffer.join(input);
}

void Connection::processData()
{
	static const String delimiter(NEWLINE);

	if (this->__buffer.empty())
		return;

	size_t pos = this->__buffer.find(delimiter);
	if (pos == String::npos)
		return;

	if (pos > MAX_MESSAGE_SIZE)
		throw std::runtime_error("message exceeds maximum allowed size");
	String message = this->__buffer.substr(0, pos).to_string();
	this->__buffer.erase(0, pos + delimiter.length());

	mzu::info("received: \"" + message + "\"");

	// * admin and gui to be implemented
	if (__client.getConnectionType() != CONNECTION_GAME || !__server)
		return;

	GameServer *gs = static_cast<GameServer *>(__server);
	Game *game = gs->getGame();
	if (!game)
		return;

	if (__client.getState() == PLAYER_HANDSHAKE)
	{
		int playerId = -1;
		String response = game->handleHandshake(message, playerId);
		if (playerId >= 0)
		{
			__client.setPlayerId(playerId);
			__client.setState(PLAYER_ALIVE);
		}
		else
		{
			__client.setState(PLAYER_DEAD);
		}
		__responseQueue.push(BasicString(response));
	}
	else if (__client.getState() == PLAYER_ALIVE)
	{
		if (game->canAcceptCommand(__client.getPlayerId()))
			game->enqueueCommand(__client.getPlayerId(), message);
	}
}

/*****************************************************************************
 *								OUTPUT QUEUE								 *
 *****************************************************************************/

bool Connection::hasPendingOutput() const
{
	return !this->__responseQueue.empty();
}

const BasicString &Connection::frontOutput() const
{
	return this->__responseQueue.front();
}

void Connection::popOutput()
{
	this->__responseQueue.pop();
}

void Connection::pushOutput(const String &msg)
{
	__responseQueue.push(BasicString(msg));
}

Client &Connection::getClient()
{
	return __client;
}

const Client &Connection::getClient() const
{
	return __client;
}
