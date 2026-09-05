#include "ConnectionGame.hpp"
#include "../ServerManager/ServerGame.hpp"
#include "Game.hpp"

ConnectionGame::ConnectionGame(Server *server) : 	Connection(server),
													__state(PLAYER_HANDSHAKE),
													__playerId(-1)
{
	__responseQueue.push(BasicString("BIENVENUE\n"));
	mzu::debug("ConnectionGame constructor");
}
ConnectionGame::ConnectionGame(const ConnectionGame &copy) : Connection(copy)
{
	mzu::debug("ConnectionGame copy constructor");
	*this = copy;
}
ConnectionGame &ConnectionGame::operator=(const ConnectionGame &assign)
{
	mzu::debug("ConnectionGame copy assignement operator");
	if (this != &assign)
	{
		__state = assign.__state;
		__playerId = assign.__playerId;
	}
	return *this;
}
ConnectionGame::~ConnectionGame()
{
	mzu::debug("ConnectionGame destructor");
}

bool ConnectionGame::readSocket()
{
	char buff[READ_SIZE + 1];

	ssize_t bytesRead = recv(this->__sd, buff, READ_SIZE, 0);
	if (bytesRead == 0)
	{
		return false;
	}
	else if (bytesRead > 0)
	{
		buff[bytesRead] = '\0';
		this->addData(BasicString(buff, bytesRead));
	}
	else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
		return false;
	return true;
}

bool ConnectionGame::writeSocket()
{
	if (this->getState() == PLAYER_DEAD && !this->hasPendingOutput())
		return false;
	if (!this->hasPendingOutput())
		return true;

	BasicString &out = __responseQueue.front();
	if (out.empty())
	{
		popOutput();
		return true;
	}
	ssize_t bytesWritten = send(__sd, out.getBuff(), out.length(), MSG_NOSIGNAL);
	if (bytesWritten > 0)
	{
		if (static_cast<size_t>(bytesWritten) == out.length())
			popOutput();
		else
			out.erase(0, static_cast<size_t>(bytesWritten));
	}
	else if (bytesWritten == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR))
		return false;
	return true;
}

e_player_state ConnectionGame::getState() const
{
	return __state;
}

void ConnectionGame::setState(e_player_state state)
{
	__state = state;
}

int ConnectionGame::getPlayerId() const
{
	return __playerId;
}

void ConnectionGame::setPlayerId(int id)
{
	__playerId = id;
}

/*****************************************************************************
 *								MESSAGE FRAMING							     *
 *****************************************************************************/

void ConnectionGame::processMessage(const String &message)
{
    mzu::info("received: \"" + message + "\"");
	ServerGame *gs = static_cast<ServerGame *>(__server);
	Game *game = gs->getGame();
	if (!game || game->getState() != GAME_RUNNING || __state == PLAYER_DEAD)
		return;

	if (__state == PLAYER_HANDSHAKE)
	{
		int playerId = -1;
		String response = game->handleHandshake(message, playerId);
		if (playerId >= 0)
		{
			__playerId = playerId;
			__state = PLAYER_ALIVE;
		}
		else
			__state = PLAYER_DEAD;
		__responseQueue.push(BasicString(response));
	}
	else if (__state == PLAYER_ALIVE && game->canAcceptCommand(__playerId))
		game->enqueueCommand(__playerId, message);
}
