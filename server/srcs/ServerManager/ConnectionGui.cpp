#include "ConnectionGui.hpp"
#include "ServerGame.hpp"
#include "../Game/Game.hpp"
#include "../Game/Protocole.hpp"

ConnectionGui::ConnectionGui(Server *server, Game *game) : 	Connection(server),
															__game(game),
															__state(GUI_WAITING_HANDSHAKE),
															__eventCursor(0)
{
	if (!__game)
		throw std::runtime_error("gui connection: game is not initialized");
	__eventCursor = __game->getLastGuiEventSequence();
	pushOutput(Protocole::welcome());
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
	else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
		return false;
	return true;
}

bool ConnectionGui::writeSocket()
{
	if (!this->hasPendingOutput())
		return !isClosing();

	BasicString &out = __responseQueue.front();
	if (out.empty())
	{
		popOutput();
		return true;
	}
	ssize_t bytesWritten = send(this->__sd, out.getBuff(), out.length(), MSG_NOSIGNAL);
	if (bytesWritten > 0)
	{
		if (static_cast<size_t>(bytesWritten) == out.length())
			this->popOutput();
		else
			out.erase(0, static_cast<size_t>(bytesWritten));
	}
	else if (bytesWritten == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR))
		return false;
	return true;
}

/*****************************************************************************
 *								MESSAGE FRAMING							     *
 *****************************************************************************/

void ConnectionGui::processMessage(const String &message)
{
	String request = message;
	if (!request.empty() && request[request.length() - 1] == '\r')
		request.erase(request.length() - 1);
	mzu::info("received: \"" + request + "\"");
	if (__state == GUI_CLOSING)
		return;
	if (__state == GUI_WAITING_HANDSHAKE)
	{
		if (Protocole::parseVerb(request) != PROTOCOL_GRAPHIC)
		{
			pushOutput(Protocole::unknownCommand());
			__state = GUI_CLOSING;
			return;
		}
		String snapshot = getCurrentGameState();
		__eventCursor = __game->getLastGuiEventSequence();
		pushOutput(snapshot);
		__state = GUI_READY;
		return;
	}
	String response = Protocole::executeRequest(request, *__game);
	if (!response.empty())
		pushOutput(response);
}

String ConnectionGui::getCurrentGameState() const
{
	return Protocole::snapshot(*__game);
}

bool ConnectionGui::isReady() const
{
	return __state == GUI_READY;
}

bool ConnectionGui::isClosing() const
{
	return __state == GUI_CLOSING;
}

unsigned long ConnectionGui::getEventCursor() const
{
	return __eventCursor;
}

void ConnectionGui::queueGameEvent(const s_gui_event &event)
{
	if (!isReady() || event.sequence <= __eventCursor)
		return;
	pushOutput(event.payload);
	__eventCursor = event.sequence;
}
