#include "ConnectionGui.hpp"
#include "../ServerManager/ServerGame.hpp"
#include "../Game/Game.hpp"

ConnectionGui::ConnectionGui(Server *server) : Connection(server)
{
	mzu::debug("ConnectionGui constructor");
}
ConnectionGui::ConnectionGui(const ConnectionGui &copy) : Connection(copy)
{
	mzu::debug("ConnectionGui copy constructor");
	*this = copy;
}
ConnectionGui &ConnectionGui::operator=(const ConnectionGui &assign)
{
	mzu::debug("ConnectionGui copy assignement operator");
	if (this != &assign) { }
	return *this;
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