#include "ConnectionGui.hpp"
#include "ServerGame.hpp"
#include "../Game/Game.hpp"

ConnectionGui::ConnectionGui(Server *server, Game *game) : Connection(server),
															__game(game)
{
	if (!__game)
		throw std::runtime_error("gui connection: game is not initialized");
	mzu::debug("ConnectionGui constructor");
}
ConnectionGui::~ConnectionGui()
{
	mzu::debug("ConnectionGui destructor");
}

bool ConnectionGui::readSocket()
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
	else
	{
		return false;
	}
	return true;
}

bool ConnectionGui::writeSocket()
{
	if (!this->hasPendingOutput())
		return true;

	const BasicString &out = this->frontOutput();
	ssize_t bytesWritten = send(this->__sd, out.getBuff(), out.length(), 0);
	if (bytesWritten > 0)
	{
		mzu::info("response sent");
		this->popOutput();
	}
	else
	{
		return false;
	}
	return true;
}

/*****************************************************************************
 *								MESSAGE FRAMING							     *
 *****************************************************************************/

void ConnectionGui::processMessage(const String &message)
{
	mzu::info("received: \"" + message + "\"");
}

String  ConnectionGui::getCurrentGameState()
{
    return "";
}
