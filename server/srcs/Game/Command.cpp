#include "Command.hpp"

Command::Command() : __type(CMD_UNKNOWN), __executionTime(0), __playerId(-1)
{
}

Command::Command(t_command type, const String &argument, long executionTime, int playerId)
	: __type(type), __argument(argument), __executionTime(executionTime), __playerId(playerId)
{
}

Command::Command(const Command &copy)
{
	*this = copy;
}

Command &Command::operator=(const Command &assign)
{
	if (this != &assign)
	{
		__type = assign.__type;
		__argument = assign.__argument;
		__executionTime = assign.__executionTime;
		__playerId = assign.__playerId;
	}
	return *this;
}

Command::~Command()
{
}

t_command Command::getType() const
{
	return __type;
}

const String &Command::getArgument() const
{
	return __argument;
}

long Command::getExecutionTime() const
{
	return __executionTime;
}

int Command::getPlayerId() const
{
	return __playerId;
}

bool Command::isReady(long currentTick) const
{
	return currentTick >= __executionTime;
}

int Command::durationForCommand(t_command type)
{
	switch (type)
	{
		case CMD_FORK:			return 42;
		case CMD_POSE:			return 7;
		case CMD_VOIR:			return 7;
		case CMD_PREND:			return 7;
		case CMD_AVANCE:		return 7;
		case CMD_DROITE:		return 7;
		case CMD_GAUCHE:		return 7;
		case CMD_EXPULSE:		return 7;
		case CMD_BROADCAST:		return 7;
		case CMD_INVENTAIRE:	return 1;
		case CMD_CONNECT_NBR:	return 0;
		case CMD_INCANTATION:	return 300;
		default:				return 0;
	}
}
