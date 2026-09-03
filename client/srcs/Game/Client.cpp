#include "Client.hpp"

bool Client::up = false;

void Client::stop()
{
	Client::up = false;
}

Client::Client(const String &team, const String &host, int port)
	: __team(team), __host(host), __port(port), __worldWidth(0), __worldHeight(0),
	  __state(HS_WAIT_WELCOME), __hasPending(false), __incantationPending(false)
{
	mzu::debug("Client constructor");
}
Client::Client(const Client &copy) : __port(-1), __worldWidth(0), __worldHeight(0),
	__state(HS_WAIT_WELCOME), __hasPending(false), __incantationPending(false)
{
	mzu::debug("Client copy constructor");
	*this = copy;
}
Client &Client::operator=(const Client &assign)
{
	mzu::debug("Client copy assignement operator");
	if (this != &assign)
	{
		__team = assign.__team;
		__host = assign.__host;
		__port = assign.__port;
	}
	return *this;
}
Client::~Client()
{
	mzu::debug("Client destructor");
}

/****************************************************************************
 *									MAIN LOOP								*
 ****************************************************************************/

void Client::run()
{
	mzu::info("connecting to " + __host + ":" + mzu::intToString(__port) + " as team \"" + __team + "\"");
	__connection.connectTo(__host, __port);

	Client::up = true;
	while (Client::up && __state != HS_DEAD)
	{
		int sd = __connection.getSocket();
		fd_set readSet, writeSet;
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_SET(sd, &readSet);
		if (__connection.hasPendingOutput())
			FD_SET(sd, &writeSet);

		struct timeval tv;
		tv.tv_sec = SELECT_TIMEOUT / 1000;
		tv.tv_usec = (SELECT_TIMEOUT % 1000) * 1000;

		int rv = select(sd + 1, &readSet, &writeSet, NULL, &tv);
		if (rv < 0)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("select syscall failed");
		}
		if (rv == 0)
			continue;
		if (FD_ISSET(sd, &writeSet) && !__connection.writeSocket())
			break;
		if (FD_ISSET(sd, &readSet) && !__connection.readSocket())
		{
			mzu::info("server closed the connection");
			break;
		}
		while (__connection.hasInboundLine())
		{
			String line = __connection.popInboundLine();
			mzu::debug("received: \"" + line + "\"");
			if (__state == HS_WAIT_WELCOME)
				onWelcome(line);
			else if (__state == HS_WAIT_SLOTS)
				onSlots(line);
			else if (__state == HS_WAIT_SIZE)
				onSize(line);
			else
				handlePlayingLine(line);
		}
	}
}

/****************************************************************************
 *									HANDSHAKE								*
 ****************************************************************************/

void Client::onWelcome(const String &line)
{
	(void)line;
	__connection.pushOutput(__team);
	__state = HS_WAIT_SLOTS;
}

void Client::onSlots(const String &line)
{
	if (line == "ko")
	{
		mzu::fatal("team \"" + __team + "\" refused by the server (unknown team or no free slot)");
		__state = HS_DEAD;
		return;
	}
	mzu::info("remaining team slots: " + line);
	__state = HS_WAIT_SIZE;
}

void Client::onSize(const String &line)
{
	t_svec tokens = mzu::splitBySpaces(line);
	if (tokens.size() == 2)
	{
		__worldWidth = static_cast<int>(mzu::stringToInt(tokens[0]));
		__worldHeight = static_cast<int>(mzu::stringToInt(tokens[1]));
	}
	mzu::info("world size: " + mzu::intToString(__worldWidth) + "x" + mzu::intToString(__worldHeight));
	__state = HS_PLAYING;
	sendNextCommand();
}

/****************************************************************************
 *									GAMEPLAY								*
 ****************************************************************************/

void Client::sendCommand(const String &commandLine)
{
	t_svec tokens = mzu::splitBySpaces(commandLine);
	if (tokens.empty())
		return;

	e_command type = CMD_UNKNOWN;
	String word = tokens[0];
	if (word == "avance")				type = CMD_AVANCE;
	else if (word == "droite")			type = CMD_DROITE;
	else if (word == "gauche")			type = CMD_GAUCHE;
	else if (word == "voir")			type = CMD_VOIR;
	else if (word == "inventaire")		type = CMD_INVENTAIRE;
	else if (word == "prend")			type = CMD_PREND;
	else if (word == "pose")			type = CMD_POSE;
	else if (word == "expulse")		type = CMD_EXPULSE;
	else if (word == "broadcast")		type = CMD_BROADCAST;
	else if (word == "incantation")	type = CMD_INCANTATION;
	else if (word == "fork")			type = CMD_FORK;
	else if (word == "connect_nbr")	type = CMD_CONNECT_NBR;

	mzu::debug("sending: \"" + commandLine + "\"");
	__connection.pushOutput(commandLine);

	if (type == CMD_INCANTATION)
		__incantationPending = true;
	else
	{
		__pending.type = type;
		__pending.argument = (tokens.size() > 1) ? tokens[1] : "";
		__hasPending = true;
	}
}

void Client::sendNextCommand()
{
	if (__hasPending || __incantationPending)
		return;
	sendCommand(__strategy.decideNextCommand());
}

void Client::handlePlayingLine(const String &line)
{
	int direction;
	int level;
	String text;

	if (Protocol::isDeath(line))
	{
		mzu::fatal("died");
		__state = HS_DEAD;
		return;
	}
	if (Protocol::parseBroadcast(line, direction, text))
	{
		mzu::info("heard from direction " + mzu::intToString(direction) + ": " + text);
		return;
	}
	if (Protocol::parseDeplacement(line, direction))
	{
		mzu::info("kicked, came from direction " + mzu::intToString(direction));
		return;
	}
	if (Protocol::isElevationEnCours(line))
	{
		mzu::info("incantation started");
		return;
	}
	if (Protocol::parseNiveauActuel(line, level))
	{
		/* Reaches every player standing on the incantation tile, not just
		 * whoever triggered it, so this can arrive with no incantation of
		 * our own in flight. */
		__strategy.applyLevel(level);
		if (__incantationPending)
		{
			__incantationPending = false;
			sendNextCommand();
		}
		return;
	}
	if (__incantationPending)
	{
		mzu::info("incantation failed: " + line);
		__incantationPending = false;
		sendNextCommand();
		return;
	}

	if (!__hasPending)
		return;
	__hasPending = false;
	handleGenericReply(line);
	sendNextCommand();
}

void Client::handleGenericReply(const String &line)
{
	switch (__pending.type)
	{
		case CMD_VOIR:
			__strategy.applyVision(line);
			break;
		case CMD_INVENTAIRE:
			__strategy.applyInventory(line);
			break;
		case CMD_PREND:
			__strategy.applyPrendResult(Protocol::resourceFromName(__pending.argument), line == "ok");
			break;
		case CMD_POSE:
			__strategy.applyPoseResult(Protocol::resourceFromName(__pending.argument), line == "ok");
			break;
		case CMD_AVANCE:
		case CMD_DROITE:
		case CMD_GAUCHE:
			__strategy.invalidateVision();
			break;
		case CMD_CONNECT_NBR:
			mzu::info("unused connections: " + line);
			break;
		default:
			break;
	}
}
