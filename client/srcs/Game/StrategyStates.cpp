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

	// Fork early if food allows
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

	// Avoid disturbing occupied ritual tiles
	bool isTileIncantationReady = ElevationManager::isTileResourceComplete(ctx->getLevel(), here);
	bool leaderActive = ctx->hasActiveLeaderForLevel(ctx->getLevel());
	if (here.players > 1 && (leaderActive || isTileIncantationReady))
		return "avance";

	// Pick up food if needed
	if (here.resources[NOURRITURE] > 0 && inv[NOURRITURE] < 45)
		return "prend " + Protocol::resourceName(NOURRITURE);

	// Fork early if food allows
	if (ctx->canLayEgg() && inv[NOURRITURE] >= 14)
	{
		ctx->recordEggLaid();
		ctx->setQueuedCommand("broadcast " + ctx->getMessenger().buildEgg(ctx->getMyId()));
		mzu::info("Early egg laid (total eggs=" + mzu::intToString(ctx->getTotalTeamEggs()) + ")");
		return "fork";
	}

	// Pick up stones if tile is not reserved for incantation
	if (here.players <= 1 && !isTileIncantationReady)
	{
		for (size_t i = 0; i < STONE_COUNT; i++)
		{
			if (here.resources[STONES[i]] > 0)
				return "prend " + Protocol::resourceName(STONES[i]);
		}
	}

	// Assume leadership if eligible and cooldown expired
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
			ctx->clearEnrolledFollowers();
			ctx->resetGatherTimeout();
			ctx->resetGatherWaitTicks();
			ctx->resetBroadcastTimer();
			mzu::info("Assuming Leader role for Level " + mzu::intToString(ctx->getLevel()) + " (has stones!)");
			return "voir";
		}
	}

	// Food priority when low
	if (inv[NOURRITURE] < 12)
	{
		for (size_t i = 1; i < vision.size(); i++)
		{
			if (vision[i].resources[NOURRITURE] > 0)
				return SoundNavigator::stepTowardVisionTile(i, ctx);
		}
	}

	const s_elevation_req &req = Elevation::getRequirement(ctx->getLevel());

	// Prioritize missing elevation stones
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

	// Seek visible resources
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

	// Pick up food on tile
	if (here.resources[NOURRITURE] > 0 && inv[NOURRITURE] < 40)
		return "prend " + Protocol::resourceName(NOURRITURE);

	// Abort if low on food
	if (inv[NOURRITURE] < 8)
	{
		mzu::info("Low food while leading gather, aborting to forage");
		ctx->transitionTo(ctx->getStateForage());
		ctx->setActiveLeaderForLevel(ctx->getLevel(), -1);
		ctx->clearEnrolledFollowers();
		return "broadcast " + ctx->getMessenger().buildAbort(ctx->getMyId(), ctx->getLevel());
	}

	// Deposit required stones
	int missing[RESOURCE_COUNT];
	ElevationManager::calculateMissingStones(ctx->getLevel(), here, missing);

	e_resource toDeposit = ElevationManager::getStoneToDeposit(inv, missing);
	if (toDeposit != RESOURCE_COUNT)
		return "pose " + Protocol::resourceName(toDeposit);

	// Check if all requirements met on tile
	bool stonesComplete = ElevationManager::isTileResourceComplete(ctx->getLevel(), here);
	int targetPlayers = Elevation::getRequirement(ctx->getLevel()).players_needed;
	int neededFollowers = targetPlayers - 1;
	bool followersConfirmed = (neededFollowers <= 0) || (ctx->getConfirmedFollowersCount() >= neededFollowers);
	bool playersComplete = (here.players == targetPlayers) && (followersConfirmed || ctx->getGatherWaitTicks() > 10);

	if (stonesComplete && playersComplete)
	{
		mzu::info("All requirements met on tile! Firing FULL, INCANT, and starting incantation!");
		ctx->transitionTo(ctx->getStateIncanting());
		ctx->setQueuedCommand("incantation");
		return "broadcast " + ctx->getMessenger().buildFull(ctx->getMyId(), ctx->getLevel());
	}

	// Prune unconfirmed enrolled followers
	if (ctx->getGatherWaitTicks() > 30 && ctx->getGatherWaitTicks() % 15 == 0)
	{
		ctx->pruneUnconfirmedEnrolled();
	}

	// Abort gather if players gathered but stones remain incomplete
	if (here.players >= targetPlayers && !stonesComplete && ctx->incrementGatherWaitTicks() > 40)
	{
		mzu::info("All players on tile but ritual stones incomplete after 40 ticks! Aborting gather to forage.");
		ctx->resetGatherTimeout();
		ctx->resetGatherWaitTicks();
		ctx->setGatherCooldown(60);
		ctx->transitionTo(ctx->getStateForage());
		ctx->setActiveLeaderForLevel(ctx->getLevel(), -1);
		ctx->clearEnrolledFollowers();
		return "broadcast " + ctx->getMessenger().buildAbort(ctx->getMyId(), ctx->getLevel());
	}

	// Timeout if waiting too long for followers
	if (ctx->incrementGatherTimeout() > 120)
	{
		mzu::info("Gathering timed out (120 ticks). Returning to FORAGE.");
		ctx->resetGatherTimeout();
		ctx->resetGatherWaitTicks();
		ctx->setGatherCooldown(60);
		ctx->transitionTo(ctx->getStateForage());
		ctx->setActiveLeaderForLevel(ctx->getLevel(), -1);
		ctx->clearEnrolledFollowers();
		return "broadcast " + ctx->getMessenger().buildAbort(ctx->getMyId(), ctx->getLevel());
	}

	// Broadcast OFFER with missing stones and needed slots
	int slotsNeeded = targetPlayers - 1 - static_cast<int>(ctx->getEnrolledFollowers().size());
	if (slotsNeeded < 0) slotsNeeded = 0;

	// Fork if waiting for players and team size is below cap
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
		return "broadcast " + ctx->getMessenger().buildOffer(ctx->getMyId(), ctx->getLevel(), missing, slotsNeeded, ctx->getEnrolledFollowers());
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
		ctx->setEnrolled(false);
		return "voir";
	}

	int dir = ctx->getTargetLeaderDirection();

	if (dir > 0)
	{
		ctx->setTargetLeaderDirection(-1);
		return SoundNavigator::stepSoundDirection(dir, ctx);
	}
	else if (dir == 0)
	{
		const std::vector<t_tile_content> &vision = ctx->getVision();
		if (vision.empty())
			return "voir";

		const t_tile_content &here = vision[0];
		const int *inv = ctx->getInventory();
		int targetPlayers = Elevation::getRequirement(ctx->getLevel()).players_needed;

		// Step off if not enrolled in squad
		if (!ctx->isEnrolled() && ctx->getGatherWaitTicks() > 2)
		{
			mzu::info("Not enrolled in squad; stepping off to forage.");
			ctx->transitionTo(ctx->getStateForage());
			ctx->setTargetLeaderId(-1);
			ctx->setTargetLeaderDirection(-1);
			ctx->setEnrolled(false);
			return "avance";
		}

		// Step off if tile exceeds required player count
		if (here.players > targetPlayers && ctx->getGatherWaitTicks() > 1)
		{
			mzu::info("Leader tile overcrowded (" + mzu::intToString(here.players) + " > " + mzu::intToString(targetPlayers) + "); stepping off");
			ctx->transitionTo(ctx->getStateForage());
			ctx->setTargetLeaderId(-1);
			ctx->setTargetLeaderDirection(-1);
			ctx->setEnrolled(false);
			return "avance";
		}

		ctx->incrementGatherWaitTicks();

		// Timeout: step off if ritual does not start
		if (ctx->getGatherWaitTicks() > 120)
		{
			mzu::info("Timed out waiting on leader tile; returning to forage");
			ctx->resetGatherWaitTicks();
			ctx->transitionTo(ctx->getStateForage());
			ctx->setTargetLeaderId(-1);
			ctx->setTargetLeaderDirection(-1);
			ctx->setEnrolled(false);
			return "avance";
		}

		// Deposit needed stones onto tile
		int missing[RESOURCE_COUNT];
		ElevationManager::calculateMissingStones(ctx->getLevel(), here, missing);

		e_resource toDeposit = ElevationManager::getStoneToDeposit(inv, missing);
		if (toDeposit != RESOURCE_COUNT)
			return "pose " + Protocol::resourceName(toDeposit);

		// Broadcast HERE to confirm arrival
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
			ctx->setEnrolled(false);
			return "avance";
		}

		// Wait on tile for ritual
		return "voir";
	}

	// Wait on current tile between audio signals
	return "voir";
}

/*************************************************************************
 *                          STATE INCANTING                              *
 *************************************************************************/

String StateIncanting::execute(Strategy *ctx)
{
	// Incantation timeout fallback
	if (ctx->incrementGatherWaitTicks() > 320)
	{
		mzu::info("Incantation timed out (320 ticks without level up). Returning to FORAGE.");
		ctx->resetGatherWaitTicks();
		ctx->transitionTo(ctx->getStateForage());
		return "voir";
	}
	return "voir";
}
