#include "BroadcastMessenger.hpp"
#include "../utilities/MZU.hpp"
#include <cmath>

BroadcastMessenger::BroadcastMessenger() : __teamName(""), __secretHash(0)
{
}

BroadcastMessenger::BroadcastMessenger(const String &team)
{
	setTeam(team);
}

BroadcastMessenger::~BroadcastMessenger()
{
}

int BroadcastMessenger::computeHash(const String &team) const
{
	if (team.empty())
		return 0;
	long long hash = 42;
	for (size_t i = 0; i < team.size(); i++)
		hash = (hash * 31 + static_cast<unsigned char>(team[i])) ^ 0x5F37;
	return static_cast<int>(std::abs(hash % 90000) + 10000); // 5-digit number
}

void BroadcastMessenger::setTeam(const String &team)
{
	__teamName = team;
	__secretHash = computeHash(team);
}

int BroadcastMessenger::getSecretHash() const
{
	return __secretHash;
}

String BroadcastMessenger::buildOffer(int myId, int level, const int missingStones[RESOURCE_COUNT], int slotsNeeded) const
{
	String msg = "ZAP " + mzu::intToString(__secretHash) + " " + mzu::intToString(myId) + " OFFER "
				+ mzu::intToString(level) + " " + mzu::intToString(slotsNeeded);
	for (int i = 1; i < RESOURCE_COUNT; i++)
		msg += " " + mzu::intToString(missingStones[i]);
	return msg;
}

String BroadcastMessenger::buildJoin(int myId, int level, int leaderId) const
{
	return "ZAP " + mzu::intToString(__secretHash) + " " + mzu::intToString(myId) + " JOIN "
			+ mzu::intToString(level) + " " + mzu::intToString(leaderId);
}

String BroadcastMessenger::buildHere(int myId, int level, int leaderId) const
{
	return "ZAP " + mzu::intToString(__secretHash) + " " + mzu::intToString(myId) + " HERE "
			+ mzu::intToString(level) + " " + mzu::intToString(leaderId);
}

String BroadcastMessenger::buildFull(int myId, int level) const
{
	return "ZAP " + mzu::intToString(__secretHash) + " " + mzu::intToString(myId) + " FULL "
			+ mzu::intToString(level);
}

String BroadcastMessenger::buildIncant(int myId, int level) const
{
	return "ZAP " + mzu::intToString(__secretHash) + " " + mzu::intToString(myId) + " INCANT "
			+ mzu::intToString(level);
}

String BroadcastMessenger::buildAbort(int myId, int level) const
{
	return "ZAP " + mzu::intToString(__secretHash) + " " + mzu::intToString(myId) + " ABORT "
			+ mzu::intToString(level);
}

String BroadcastMessenger::buildEgg(int myId) const
{
	return "ZAP " + mzu::intToString(__secretHash) + " " + mzu::intToString(myId) + " EGG 0";
}

bool BroadcastMessenger::parse(const String &rawText, s_parsed_msg &outMsg) const
{
	outMsg.valid = false;
	outMsg.cmd = MSG_NONE;
	for (int i = 0; i < RESOURCE_COUNT; i++)
		outMsg.missingStones[i] = 0;

	t_svec tokens = mzu::splitBySpaces(rawText);
	if (tokens.size() < 5)
		return false;

	if (tokens[0] != "ZAP")
		return false;

	int hash = static_cast<int>(mzu::stringToInt(tokens[1]));
	if (hash != __secretHash)
		return false;

	outMsg.senderId = static_cast<int>(mzu::stringToInt(tokens[2]));
	String cmdStr = tokens[3];
	outMsg.targetLevel = static_cast<int>(mzu::stringToInt(tokens[4]));

	if (cmdStr == "OFFER")
	{
		outMsg.cmd = MSG_OFFER;
		if (tokens.size() >= 6)
			outMsg.slotsNeeded = static_cast<int>(mzu::stringToInt(tokens[5]));
		for (int i = 1; i < RESOURCE_COUNT && (5 + i) < static_cast<int>(tokens.size()); i++)
			outMsg.missingStones[i] = static_cast<int>(mzu::stringToInt(tokens[5 + i]));
		outMsg.valid = true;
		return true;
	}
	else if (cmdStr == "JOIN")
	{
		outMsg.cmd = MSG_JOIN;
		if (tokens.size() >= 6)
			outMsg.leaderId = static_cast<int>(mzu::stringToInt(tokens[5]));
		outMsg.valid = true;
		return true;
	}
	else if (cmdStr == "HERE")
	{
		outMsg.cmd = MSG_HERE;
		if (tokens.size() >= 6)
			outMsg.leaderId = static_cast<int>(mzu::stringToInt(tokens[5]));
		outMsg.valid = true;
		return true;
	}
	else if (cmdStr == "FULL")
	{
		outMsg.cmd = MSG_FULL;
		outMsg.valid = true;
		return true;
	}
	else if (cmdStr == "INCANT")
	{
		outMsg.cmd = MSG_INCANT;
		outMsg.valid = true;
		return true;
	}
	else if (cmdStr == "ABORT")
	{
		outMsg.cmd = MSG_ABORT;
		outMsg.valid = true;
		return true;
	}
	else if (cmdStr == "EGG")
	{
		outMsg.cmd = MSG_EGG;
		outMsg.valid = true;
		return true;
	}

	return false;
}
