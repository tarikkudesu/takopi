#include "StrategyStates.hpp"
#include "Strategy.hpp"
#include "../utilities/MZU.hpp"

static const e_resource STONES[] = {LINEMATE, DERAUMERE, SIBUR, MENDIANE, PHIRAS, THYSTAME};
static const size_t STONE_COUNT = sizeof(STONES) / sizeof(STONES[0]);

/*************************************************************************
 *                              STATE SOLO                               *
 *************************************************************************/

String StateSolo::execute(Strategy *ctx)
{
	if (ctx->getLevel() > 1)
	{
		ctx->transitionTo(ctx->getStateForage());
		return "voir";
	}

	const std::vector<t_tile_content> &vision = ctx->getVision();
	if (vision.empty())
		return "voir";

	const t_tile_content &here = vision[0];
	const int *inv = ctx->getInventory();

	// Proactively lay early eggs to reach target team size of 8 as fast as possible!
	if (ctx->canLayEgg() && inv[NOURRITURE] >= 14)
	{
		ctx->recordEggLaid();
		ctx->setQueuedCommand("broadcast " + ctx->getMessenger().buildEgg(ctx->getMyId()));
		mzu::info("Proactive early egg laid (fork) to reach target team size of 8 (total eggs=" + mzu::intToString(ctx->getTotalTeamEggs()) + ")");
		return "fork";
	}

	if (here.resources[LINEMATE] >= 1 && inv[NOURRITURE] >= 9)
	{
		if (here.players > 1)
			return "avance";
		return "incantation";
	}

	if (inv[LINEMATE] >= 1)
	{
		if (here.resources[LINEMATE] < 1)
			return "pose " + Protocol::resourceName(LINEMATE);
		if (here.players > 1)
			return "avance";
		return "incantation";
	}

	if (here.resources[LINEMATE] > 0)
		return "prend " + Protocol::resourceName(LINEMATE);

	if (here.resources[NOURRITURE] > 0 && inv[NOURRITURE] < 20)
		return "prend " + Protocol::resourceName(NOURRITURE);

	for (size_t i = 1; i < vision.size(); i++)
	{
		if (vision[i].resources[LINEMATE] > 0)
			return SoundNavigator::stepTowardVisionTile(i, ctx);
	}

	for (size_t i = 1; i < vision.size(); i++)
	{
		if (vision[i].resources[NOURRITURE] > 0)
			return SoundNavigator::stepTowardVisionTile(i, ctx);
	}

	return SoundNavigator::wander(ctx);
}

/*************************************************************************
 *                            STATE SURVIVE                              *
 *************************************************************************/

String StateSurvive::execute(Strategy *ctx)
{
	const int *inv = ctx->getInventory();
	if (inv[NOURRITURE] >= 18)
	{
		if (ctx->getLevel() == 1)
			ctx->transitionTo(ctx->getStateSolo());
		else
			ctx->transitionTo(ctx->getStateForage());
		return "voir";
	}

	const std::vector<t_tile_content> &vision = ctx->getVision();
	const t_tile_content &here = vision[0];
	if (here.resources[NOURRITURE] > 0)
		return "prend " + Protocol::resourceName(NOURRITURE);

	for (size_t i = 1; i < vision.size(); i++)
	{
		if (vision[i].resources[NOURRITURE] > 0)
			return SoundNavigator::stepTowardVisionTile(i, ctx);
	}

	return SoundNavigator::wander(ctx);
}

/*************************************************************************
 *                            STATE FORAGE                               *
 *************************************************************************/

String StateForage::execute(Strategy *ctx)
{
	if (ctx->getLevel() == 1)
	{
		ctx->transitionTo(ctx->getStateSolo());
		return "voir";
	}

	const std::vector<t_tile_content> &vision = ctx->getVision();
	if (vision.empty())
		return "voir";

	const t_tile_content &here = vision[0];
	const int *inv = ctx->getInventory();

	// 1. Pick up food if available and needed
	if (here.resources[NOURRITURE] > 0 && inv[NOURRITURE] < 45)
		return "prend " + Protocol::resourceName(NOURRITURE);

	// 2. Proactively lay early eggs to reach target team size of 8 as fast as possible!
	if (ctx->canLayEgg() && inv[NOURRITURE] >= 14)
	{
		ctx->recordEggLaid();
		ctx->setQueuedCommand("broadcast " + ctx->getMessenger().buildEgg(ctx->getMyId()));
		mzu::info("Proactive early egg laid (fork) to reach target team size of 8 (total eggs=" + mzu::intToString(ctx->getTotalTeamEggs()) + ")");
		return "fork";
	}

	// 3. Pick up stones unless this is an active leader's ritual tile
	bool isTileIncantationReady = ElevationManager::isTileResourceComplete(ctx->getLevel(), here);
	bool leaderActive = ctx->hasActiveLeaderForLevel(ctx->getLevel());
	int activeLeaderId = ctx->getActiveLeaderForLevel(ctx->getLevel());
	bool isLeaderTile = (leaderActive && activeLeaderId != ctx->getMyId() && ctx->getTargetLeaderDirection() == 0);

	if (!isLeaderTile && !isTileIncantationReady)
	{
		for (size_t i = 0; i < STONE_COUNT; i++)
		{
			if (here.resources[STONES[i]] > 0)
				return "prend " + Protocol::resourceName(STONES[i]);
		}
	}

	// 4. Assume leadership ONLY if no leader is currently active for our level and cooldown expired
	if (inv[NOURRITURE] >= 10 && !leaderActive && ctx->getGatherCooldown() == 0)
	{
		bool hasStones = ElevationManager::hasAllElevationStones(ctx->getLevel(), inv);
		if (!hasStones && ctx->getLevel() >= 2)
			hasStones = ElevationManager::hasEnoughToLead(ctx->getLevel(), inv);

		if (hasStones)
		{
			ctx->transitionTo(ctx->getStateLeaderGather());
			ctx->setActiveLeaderForLevel(ctx->getLevel(), ctx->getMyId());
			ctx->clearConfirmedFollowers();
			ctx->resetGatherTimeout();
			ctx->resetGatherWaitTicks();
			ctx->resetBroadcastTimer();
			mzu::info("Assuming Leader role for Level " + mzu::intToString(ctx->getLevel()) + " (has stones!)");
			return "voir";
		}
	}

	// 5. If this is an active ritual tile or a complete tile, step away to not disturb it
	if (here.players > 1 && (isLeaderTile || isTileIncantationReady))
		return "avance";

	// 6. Food priority if low
	if (inv[NOURRITURE] < 12)
	{
		for (size_t i = 1; i < vision.size(); i++)
		{
			if (vision[i].resources[NOURRITURE] > 0)
				return SoundNavigator::stepTowardVisionTile(i, ctx);
		}
	}

	const s_elevation_req &req = Elevation::getRequirement(ctx->getLevel());

	// 7. Prioritize stepping towards missing elevation stones
	for (size_t i = 1; i < vision.size(); i++)
	{
		for (size_t s = 0; s < STONE_COUNT; s++)
		{
			e_resource stone = STONES[s];
			int reqAmount = Elevation::requiredAmount(req, stone);
			if (inv[stone] < reqAmount && vision[i].resources[stone] > 0)
				return SoundNavigator::stepTowardVisionTile(i, ctx);
		}
	}

	// 8. Seek any stone or food in vision
	for (size_t i = 1; i < vision.size(); i++)
	{
		for (size_t s = 0; s < STONE_COUNT; s++)
		{
			if (vision[i].resources[STONES[s]] > 0)
				return SoundNavigator::stepTowardVisionTile(i, ctx);
		}
		if (vision[i].resources[NOURRITURE] > 0)
			return SoundNavigator::stepTowardVisionTile(i, ctx);
	}

	return SoundNavigator::wander(ctx);
}

/*************************************************************************
 *                         STATE LEADER GATHER                           *
 *************************************************************************/

String StateLeaderGather::execute(Strategy *ctx)
{
	const std::vector<t_tile_content> &vision = ctx->getVision();
	if (vision.empty())
		return "voir";

	const t_tile_content &here = vision[0];
	const int *inv = ctx->getInventory();

	// 1. Pick up extra food if available on current tile
	if (here.resources[NOURRITURE] > 0 && inv[NOURRITURE] < 40)
		return "prend " + Protocol::resourceName(NOURRITURE);

	// Low food safety: abort gather and forage if food is dangerously low
	if (inv[NOURRITURE] < 8)
	{
		mzu::info("Low food while leading gather, aborting to forage");
		ctx->transitionTo(ctx->getStateForage());
		ctx->setActiveLeaderForLevel(ctx->getLevel(), -1);
		return "broadcast " + ctx->getMessenger().buildAbort(ctx->getMyId(), ctx->getLevel());
	}

	// 2. Deposit required stones
	int missing[RESOURCE_COUNT];
	ElevationManager::calculateMissingStones(ctx->getLevel(), here, missing);

	e_resource toDeposit = ElevationManager::getStoneToDeposit(inv, missing);
	if (toDeposit != RESOURCE_COUNT)
		return "pose " + Protocol::resourceName(toDeposit);

	// 3. Check if all requirements met on tile
	bool stonesComplete = ElevationManager::isTileResourceComplete(ctx->getLevel(), here);
	int targetPlayers = Elevation::getRequirement(ctx->getLevel()).players_needed;
	int neededFollowers = targetPlayers - 1;
	bool followersConfirmed = (neededFollowers <= 0) || (ctx->getConfirmedFollowersCount() >= neededFollowers);
	bool playersComplete = (here.players == targetPlayers) && followersConfirmed;

	if (stonesComplete && playersComplete)
	{
		mzu::info("All requirements met on tile! Firing FULL, INCANT, and starting incantation!");
		ctx->transitionTo(ctx->getStateIncanting());
		ctx->setQueuedCommand("incantation");
		return "broadcast " + ctx->getMessenger().buildFull(ctx->getMyId(), ctx->getLevel());
	}

	// If all players are on tile, but stones are NOT complete after 40 ticks:
	// Abort so teammates forage for missing stones.
	if (here.players >= targetPlayers && !stonesComplete && ctx->incrementGatherWaitTicks() > 40)
	{
		mzu::info("All players on tile but ritual stones incomplete after 40 ticks! Aborting gather to forage.");
		ctx->resetGatherTimeout();
		ctx->resetGatherWaitTicks();
		ctx->setGatherCooldown(60);
		ctx->transitionTo(ctx->getStateForage());
		ctx->setActiveLeaderForLevel(ctx->getLevel(), -1);
		return "broadcast " + ctx->getMessenger().buildAbort(ctx->getMyId(), ctx->getLevel());
	}

	// 4. Timeout safety: abort if stuck waiting for teammates after 120 ticks
	if (ctx->incrementGatherTimeout() > 120)
	{
		mzu::info("Gathering timed out (120 ticks). Returning to FORAGE.");
		ctx->resetGatherTimeout();
		ctx->resetGatherWaitTicks();
		ctx->setGatherCooldown(60);
		ctx->transitionTo(ctx->getStateForage());
		ctx->setActiveLeaderForLevel(ctx->getLevel(), -1);
		return "broadcast " + ctx->getMessenger().buildAbort(ctx->getMyId(), ctx->getLevel());
	}

	// 5. Broadcast OFFER to guide teammates to this tile
	int slotsNeeded = targetPlayers - here.players;
	if (slotsNeeded < 0) slotsNeeded = 0;

	// If missing teammates to elevate and nobody has arrived after waiting, lay AT MOST 1 egg if team size < 8!
	if (slotsNeeded > 0 && inv[NOURRITURE] >= 14 && ctx->canLayEgg() && ctx->getGatherTimeout() > 20)
	{
		ctx->recordEggLaid();
		ctx->setQueuedCommand("broadcast " + ctx->getMessenger().buildEgg(ctx->getMyId()));
		mzu::info("Leader laying 1 egg (fork) to reach target team size of 8 (current estimate=" + mzu::intToString(ctx->getTeamSizeEstimate()) + ")");
		return "fork";
	}

	if (ctx->incrementBroadcastTimer() >= 2)
	{
		ctx->resetBroadcastTimer();
		return "broadcast " + ctx->getMessenger().buildOffer(ctx->getMyId(), ctx->getLevel(), missing, slotsNeeded);
	}

	return "voir";
}

/*************************************************************************
 *                          STATE JOIN LEADER                            *
 **************************************************************************/

String StateJoinLeader::execute(Strategy *ctx)
{
	if (ctx->incrementLeaderHeartbeat() > 100)
	{
		mzu::info("Lost contact with Leader. Returning to FORAGE.");
		ctx->transitionTo(ctx->getStateForage());
		ctx->setTargetLeaderId(-1);
		ctx->setTargetLeaderDirection(-1);
		return "voir";
	}

	int dir = ctx->getTargetLeaderDirection();

	if (dir > 0)
	{
		ctx->setTargetLeaderDirection(-1);
		return SoundNavigator::stepSoundDirection(dir, ctx);
	}
	else if (dir == 0 || ctx->getTargetLeaderDirection() == 0)
	{
		const std::vector<t_tile_content> &vision = ctx->getVision();
		if (vision.empty())
			return "voir";

		const t_tile_content &here = vision[0];
		const int *inv = ctx->getInventory();
		int targetPlayers = Elevation::getRequirement(ctx->getLevel()).players_needed;

		// If we just arrived on the tile and it's already full: we are extra, step off!
		if (here.players > targetPlayers && ctx->getGatherWaitTicks() == 0)
		{
			mzu::info("Leader tile already full on arrival (" + mzu::intToString(here.players) + " > " + mzu::intToString(targetPlayers) + "); stepping off!");
			ctx->transitionTo(ctx->getStateForage());
			ctx->setTargetLeaderId(-1);
			ctx->setTargetLeaderDirection(-1);
			return "avance";
		}

		ctx->incrementGatherWaitTicks();

		// Timeout safety: if on tile for 120 ticks and ritual hasn't started, step off!
		if (ctx->getGatherWaitTicks() > 120)
		{
			mzu::info("Waited 120 ticks on leader tile with no incantation started. Stepping off to FORAGE!");
			ctx->resetGatherWaitTicks();
			ctx->transitionTo(ctx->getStateForage());
			ctx->setTargetLeaderId(-1);
			ctx->setTargetLeaderDirection(-1);
			return "avance";
		}

		// 1. Deposit needed stones FIRST so the tile is ready for ritual immediately!
		int missing[RESOURCE_COUNT];
		ElevationManager::calculateMissingStones(ctx->getLevel(), here, missing);

		e_resource toDeposit = ElevationManager::getStoneToDeposit(inv, missing);
		if (toDeposit != RESOURCE_COUNT)
			return "pose " + Protocol::resourceName(toDeposit);

		// 2. Periodically confirm to leader that we are on tile
		if (ctx->incrementBroadcastTimer() >= 2)
		{
			ctx->resetBroadcastTimer();
			return "broadcast " + ctx->getMessenger().buildHere(ctx->getMyId(), ctx->getLevel(), ctx->getTargetLeaderId());
		}

		if (here.resources[NOURRITURE] > 0 && inv[NOURRITURE] < 40)
			return "prend " + Protocol::resourceName(NOURRITURE);

		if (inv[NOURRITURE] < 8)
		{
			mzu::info("Low food while waiting for Leader, returning to forage");
			ctx->transitionTo(ctx->getStateForage());
			ctx->setTargetLeaderId(-1);
			ctx->setTargetLeaderDirection(-1);
			return "voir";
		}

		// If resources are complete or being gathered, stay on tile waiting for incantation
		return "voir";
	}

	// Between sound pulses, look around and wait for the next audio signal without sprinting blindly!
	return "voir";
}

/*************************************************************************
 *                          STATE INCANTING                              *
 *************************************************************************/

String StateIncanting::execute(Strategy *ctx)
{
	// Incantation lasts 300 ticks. If 320 ticks pass without level up, return to FORAGE!
	if (ctx->incrementGatherWaitTicks() > 320)
	{
		mzu::info("Incantation timed out (320 ticks without level up). Returning to FORAGE.");
		ctx->resetGatherWaitTicks();
		ctx->transitionTo(ctx->getStateForage());
		return "voir";
	}
	return "voir";
}
