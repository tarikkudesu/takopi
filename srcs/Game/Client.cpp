#include "Client.hpp"

Client::Client() : __state(PLAYER_HANDSHAKE), __connectionType(CONNECTION_UNKNOWN), __playerId(-1)
{
}

Client::Client(const Client &copy)
{
	*this = copy;
}

Client &Client::operator=(const Client &assign)
{
	if (this != &assign)
	{
		__state = assign.__state;
		__connectionType = assign.__connectionType;
		__playerId = assign.__playerId;
	}
	return *this;
}

Client::~Client()
{
}

void Client::setConnectionType(e_connection_type type)
{
	__connectionType = type;
}

e_connection_type Client::getConnectionType() const
{
	return __connectionType;
}

void Client::setState(e_player_state state)
{
	__state = state;
}

e_player_state Client::getState() const
{
	return __state;
}

int Client::getPlayerId() const
{
	return __playerId;
}

void Client::setPlayerId(int id)
{
	__playerId = id;
}
