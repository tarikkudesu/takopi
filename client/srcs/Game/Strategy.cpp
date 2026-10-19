#include "Strategy.hpp"
#include "StrategyStates.hpp"
#include "../utilities/MZU.hpp"
#include <cstdlib>
#include <ctime>
#include <unistd.h>

Strategy::Strategy() : __level(1), __hasInventory(false), __hasVision(false),
						__ticksSinceInventory(999), __teamName(""),
						__forked(false), __eggsLaidCount(0), __targetLeaderId(-1),
						__targetLeaderDirection(-1), __targetLeaderLevel(-1),
						__leaderHeartbeat(0), __broadcastTimer(0), __gatherTimeout(0),
						__gatherWaitTicks(0), __gatherCooldown(0), __consecutiveTurns(0), __consecutiveAdvances(0), __totalTeamEggs(0),
						__eggLaidAtThisLevel(false)
{
	for (int i = 0; i < RESOURCE_COUNT; i++)
		__inventory[i] = 0;
	for (int i = 0; i <= WIN_LEVEL; i++)
	{
		__activeLeaderForLevel[i] = -1;
		__lastOfferTick[i] = 999;
	}
	srand(static_cast<unsigned int>(time(NULL) ^ (getpid() << 16) ^ (long)this));
	__myId = rand() % 900000 + 100000;
	initStates();
}

Strategy::Strategy(const Strategy &copy)
{
	initStates();
	*this = copy;
}

Strategy &Strategy::operator=(const Strategy &assign)
{
	if (this != &assign)
	{
		for (int i = 0; i < RESOURCE_COUNT; i++)
			__inventory[i] = assign.__inventory[i];
		__level = assign.__level;
		__hasInventory = assign.__hasInventory;
		__hasVision = assign.__hasVision;
		__ticksSinceInventory = assign.__ticksSinceInventory;
		__vision = assign.__vision;
		__teamName = assign.__teamName;
		__myId = assign.__myId;
		__forked = assign.__forked;
		__targetLeaderId = assign.__targetLeaderId;
		__targetLeaderDirection = assign.__targetLeaderDirection;
		__targetLeaderLevel = assign.__targetLeaderLevel;
		__leaderHeartbeat = assign.__leaderHeartbeat;
		__broadcastTimer = assign.__broadcastTimer;
		__gatherTimeout = assign.__gatherTimeout;
		__gatherWaitTicks = assign.__gatherWaitTicks;
		__messenger = assign.__messenger;
		__currentState = __stateSolo;
	}
	return *this;
}

Strategy::~Strategy()
{
	cleanupStates();
}

void Strategy::initStates()
{
	__stateSolo = new StateSolo();
	__stateSurvive = new StateSurvive();
	__stateForage = new StateForage();
	__stateLeaderGather = new StateLeaderGather();
	__stateJoinLeader = new StateJoinLeader();
	__stateIncanting = new StateIncanting();
	__currentState = __stateSolo;
}

void Strategy::cleanupStates()
{
	delete __stateSolo; __stateSolo = NULL;
	delete __stateSurvive; __stateSurvive = NULL;
	delete __stateForage; __stateForage = NULL;
	delete __stateLeaderGather; __stateLeaderGather = NULL;
	delete __stateJoinLeader; __stateJoinLeader = NULL;
	delete __stateIncanting; __stateIncanting = NULL;
}

void Strategy::transitionTo(IState *newState)
{
	if (newState)
		__currentState = newState;
}

IState* Strategy::getStateSolo() const { return __stateSolo; }
IState* Strategy::getStateSurvive() const { return __stateSurvive; }
IState* Strategy::getStateForage() const { return __stateForage; }
IState* Strategy::getStateLeaderGather() const { return __stateLeaderGather; }
IState* Strategy::getStateJoinLeader() const { return __stateJoinLeader; }
IState* Strategy::getStateIncanting() const { return __stateIncanting; }
IState* Strategy::getCurrentState() const { return __currentState; }

void Strategy::setTeamName(const String &team)
{
	__teamName = team;
	__messenger.setTeam(team);
}

int Strategy::getLevel() const { return __level; }
int Strategy::getMyId() const { return __myId; }
const int* Strategy::getInventory() const { return __inventory; }
const std::vector<t_tile_content>& Strategy::getVision() const { return __vision; }
bool Strategy::hasVision() const { return __hasVision; }
bool Strategy::isForked() const { return __forked; }
void Strategy::setForked(bool f) { __forked = f; }
int Strategy::getTeamSizeEstimate() const
{
	return 1 + static_cast<int>(__knownTeammates.size()) + __totalTeamEggs;
}

int Strategy::getTotalTeamEggs() const { return __totalTeamEggs; }
bool Strategy::hasEggLaidAtThisLevel() const { return __eggLaidAtThisLevel; }

bool Strategy::canLayEgg() const
{
	// Strict hard cap of 8 team members total! Initial team is 6, so max 2 eggs allowed!
	if (__totalTeamEggs >= 2)
		return false;
	if (getTeamSizeEstimate() >= 8)
		return false;
	if (__eggLaidAtThisLevel)
		return false;
	return true;
}

void Strategy::recordEggLaid()
{
	__eggsLaidCount++;
	__totalTeamEggs++;
	__eggLaidAtThisLevel = true;
}

int Strategy::getEggsLaidCount() const { return __eggsLaidCount; }
void Strategy::addKnownTeammate(int id) { if (id != __myId) __knownTeammates.insert(id); }

bool Strategy::hasActiveLeaderForLevel(int level) const
{
	if (level < 1 || level > WIN_LEVEL) return false;
	return (__activeLeaderForLevel[level] != -1 && __lastOfferTick[level] < 60);
}

int Strategy::getActiveLeaderForLevel(int level) const
{
	if (level < 1 || level > WIN_LEVEL) return -1;
	return __activeLeaderForLevel[level];
}

void Strategy::setActiveLeaderForLevel(int level, int leaderId)
{
	if (level >= 1 && level <= WIN_LEVEL)
	{
		__activeLeaderForLevel[level] = leaderId;
		__lastOfferTick[level] = 0;
	}
}
int Strategy::getConfirmedFollowersCount() const
{
	int count = 0;
	for (std::map<int, int>::const_iterator it = __confirmedFollowersTick.begin(); it != __confirmedFollowersTick.end(); ++it)
	{
		if (it->second < 40)
			count++;
	}
	return count;
}

void Strategy::addConfirmedFollower(int id) { __confirmedFollowersTick[id] = 0; }
void Strategy::clearConfirmedFollowers() { __confirmedFollowersTick.clear(); }
int Strategy::getTargetLeaderId() const { return __targetLeaderId; }
void Strategy::setTargetLeaderId(int id) { __targetLeaderId = id; }
int Strategy::getTargetLeaderDirection() const { return __targetLeaderDirection; }
void Strategy::setTargetLeaderDirection(int dir) { __targetLeaderDirection = dir; }
int Strategy::getLeaderHeartbeat() const { return __leaderHeartbeat; }
void Strategy::resetLeaderHeartbeat() { __leaderHeartbeat = 0; }
int Strategy::incrementLeaderHeartbeat() { return ++__leaderHeartbeat; }
int Strategy::incrementBroadcastTimer() { return ++__broadcastTimer; }
void Strategy::resetBroadcastTimer() { __broadcastTimer = 0; }
int Strategy::getGatherTimeout() const { return __gatherTimeout; }
void Strategy::resetGatherTimeout() { __gatherTimeout = 0; }
int Strategy::incrementGatherTimeout() { return ++__gatherTimeout; }
int Strategy::getGatherWaitTicks() const { return __gatherWaitTicks; }
void Strategy::resetGatherWaitTicks() { __gatherWaitTicks = 0; }
int Strategy::incrementGatherWaitTicks() { return ++__gatherWaitTicks; }
int Strategy::getConsecutiveTurns() const { return __consecutiveTurns; }
void Strategy::incrementConsecutiveTurns() { __consecutiveTurns++; }
void Strategy::resetConsecutiveTurns() { __consecutiveTurns = 0; }
int Strategy::getGatherCooldown() const { return __gatherCooldown; }
void Strategy::setGatherCooldown(int cd) { __gatherCooldown = cd; }
String Strategy::getQueuedCommand() const
{
	return __queuedCommands.empty() ? "" : __queuedCommands.front();
}
void Strategy::setQueuedCommand(const String &cmd)
{
	__queuedCommands.clear();
	if (!cmd.empty())
		__queuedCommands.push_back(cmd);
}
void Strategy::queueCommand(const String &cmd)
{
	if (!cmd.empty())
		__queuedCommands.push_back(cmd);
}
BroadcastMessenger& Strategy::getMessenger() { return __messenger; }

void Strategy::applyVision(const String &rawLine)
{
	__vision = Protocol::parseVision(rawLine);
	__hasVision = !__vision.empty();
}

void Strategy::invalidateVision()
{
	__hasVision = false;
}

void Strategy::applyInventory(const String &rawLine)
{
	if (Protocol::parseInventory(rawLine, __inventory))
	{
		__hasInventory = true;
		__ticksSinceInventory = 0;
	}
}

void Strategy::applyLevel(int level)
{
	if (level > __level)
	{
		if (__level >= 1 && __level <= WIN_LEVEL)
			__activeLeaderForLevel[__level] = -1;
		__level = level;
		mzu::info("LEVEL UP! Now level " + mzu::intToString(level));
		transitionTo(__stateForage);
		__targetLeaderId = -1;
		__targetLeaderDirection = -1;
		__targetLeaderLevel = -1;
		__leaderHeartbeat = 0;
		__gatherWaitTicks = 0;
		__forked = false; // Allow reproducing if stuck at a future level!
		clearConfirmedFollowers();
	}
}

void Strategy::applyPrendResult(e_resource resource, bool ok)
{
	if (!ok) { invalidateVision(); return; }
	if (resource >= 0 && resource < RESOURCE_COUNT)
	{
		__inventory[resource]++;
		if (__hasVision && !__vision.empty() && __vision[0].resources[resource] > 0)
			__vision[0].resources[resource]--;
	}
}

void Strategy::applyPoseResult(e_resource resource, bool ok)
{
	if (!ok) return;
	if (resource >= 0 && resource < RESOURCE_COUNT)
	{
		if (__inventory[resource] > 0)
			__inventory[resource]--;
		if (__hasVision && !__vision.empty())
			__vision[0].resources[resource]++;
	}
}

void Strategy::applyBroadcast(int direction, const String &text)
{
	s_parsed_msg msg;
	if (!__messenger.parse(text, msg))
		return;

	addKnownTeammate(msg.senderId);

	if (msg.cmd == MSG_EGG)
	{
		__totalTeamEggs++;
		mzu::info("Teammate " + mzu::intToString(msg.senderId) + " laid an egg. Team size estimate: " + mzu::intToString(getTeamSizeEstimate()));
		return;
	}

	if (msg.cmd == MSG_HERE)
	{
		if (direction == 0 && msg.targetLevel == __level && msg.leaderId == __myId)
		{
			addConfirmedFollower(msg.senderId);
			mzu::info("Follower " + mzu::intToString(msg.senderId) + " confirmed on tile! Total confirmed: " + mzu::intToString(getConfirmedFollowersCount() + 1));
		}
		return;
	}

	if (msg.targetLevel != __level)
		return;

	if (msg.cmd == MSG_OFFER)
	{
		setActiveLeaderForLevel(msg.targetLevel, msg.senderId);
		if (__currentState == __stateForage && msg.slotsNeeded > 0)
		{
			bool hasFoodSafe = (__inventory[NOURRITURE] >= 6);
			// If stones are missing on leader tile, only join if we carry at least one of the needed stones!
			bool canHelp = true;
			bool stonesNeeded = false;
			bool iHaveNeeded = false;
			for (int s = 0; s < RESOURCE_COUNT; s++)
			{
				if (msg.missingStones[s] > 0)
				{
					stonesNeeded = true;
					if (__inventory[s] > 0)
						iHaveNeeded = true;
				}
			}
			if (stonesNeeded && !iHaveNeeded)
				canHelp = false;
			if (hasFoodSafe && canHelp)
			{
				transitionTo(__stateJoinLeader);
				__targetLeaderId = msg.senderId;
				__targetLeaderDirection = direction;
				__targetLeaderLevel = msg.targetLevel;
				__leaderHeartbeat = 0;
				__gatherWaitTicks = 0;
				mzu::info("Joining Leader " + mzu::intToString(msg.senderId) + " for Lvl " + mzu::intToString(__level) + " (dir=" + mzu::intToString(direction) + ")");
			}
		}
		else if (__currentState == __stateJoinLeader)
		{
			if (__targetLeaderId == msg.senderId)
			{
				__targetLeaderDirection = direction;
				__leaderHeartbeat = 0;
			}
			else if (msg.slotsNeeded > 0 && (__targetLeaderId == -1 || __leaderHeartbeat > 25))
			{
				mzu::info("Switching to active Leader " + mzu::intToString(msg.senderId) + " for Lvl " + mzu::intToString(__level));
				__targetLeaderId = msg.senderId;
				__targetLeaderDirection = direction;
				__targetLeaderLevel = msg.targetLevel;
				__leaderHeartbeat = 0;
				__gatherWaitTicks = 0;
			}
		}
		else if (__currentState == __stateLeaderGather && msg.targetLevel >= 4 &&
		         getConfirmedFollowersCount() == 0 && msg.senderId > __myId && msg.slotsNeeded > 0)
		{
			mzu::info("Yielding leadership to higher ID " + mzu::intToString(msg.senderId));
			transitionTo(__stateJoinLeader);
			__targetLeaderId = msg.senderId;
			__targetLeaderDirection = direction;
			__targetLeaderLevel = msg.targetLevel;
			__leaderHeartbeat = 0;
			__gatherWaitTicks = 0;
		}
	}
	else if (msg.cmd == MSG_FULL || msg.cmd == MSG_INCANT)
	{
		if (__targetLeaderId == msg.senderId)
		{
			if (direction == 0 || __targetLeaderDirection == 0)
			{
				mzu::info("Leader announced ritual ready; standing by on tile for elevation en cours.");
			}
			else
			{
				mzu::info("Leader announced ritual ready; continuing toward leader tile (dir=" + mzu::intToString(direction) + ").");
				__targetLeaderDirection = direction;
				__leaderHeartbeat = 0;
			}
		}
	}
	else if (msg.cmd == MSG_ABORT)
	{
		if (msg.targetLevel >= 1 && msg.targetLevel <= WIN_LEVEL && __activeLeaderForLevel[msg.targetLevel] == msg.senderId)
		{
			__activeLeaderForLevel[msg.targetLevel] = -1;
		}
		if (__targetLeaderId == msg.senderId)
		{
			transitionTo(__stateForage);
			__targetLeaderId = -1;
			__targetLeaderDirection = -1;
			__gatherWaitTicks = 0;
		}
	}
}

String Strategy::decideNextCommand()
{
	for (int i = 0; i <= WIN_LEVEL; i++)
		__lastOfferTick[i]++;
	for (std::map<int, int>::iterator it = __confirmedFollowersTick.begin(); it != __confirmedFollowersTick.end(); ++it)
		it->second++;
	if (__gatherCooldown > 0)
		__gatherCooldown--;

	if (!__queuedCommands.empty())
	{
		String next = __queuedCommands.front();
		__queuedCommands.erase(__queuedCommands.begin());
		return next;
	}

	if (!__hasVision)
		return "voir";

	if (!__hasInventory || __ticksSinceInventory >= 60)
	{
		if (__hasInventory)
			__ticksSinceInventory = 0;
		return "inventaire";
	}
	__ticksSinceInventory++;

	if (__level >= WIN_LEVEL)
		return SoundNavigator::wander(this);

	// Critical survival check
	if (__inventory[NOURRITURE] < 9 && __currentState != __stateIncanting)
		transitionTo(__stateSurvive);

	// Delegate directly to the current state!
	if (__currentState)
	{
		String cmd = __currentState->execute(this);
		if (cmd == "droite" || cmd == "gauche")
		{
			__consecutiveAdvances = 0;
			incrementConsecutiveTurns();
			if (getConsecutiveTurns() >= 3)
			{
				resetConsecutiveTurns();
				return "avance"; // Force a forward step to break turn oscillation
			}
		}
		else if (cmd == "avance")
		{
			resetConsecutiveTurns();
			__consecutiveAdvances++;
		}
		else
		{
			resetConsecutiveTurns();
		}
		return cmd;
	}

	return "voir";
}
