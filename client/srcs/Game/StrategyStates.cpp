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
	const t_tile_content &here = vision[0];
	const int *inv = ctx->getInventory();

	// Level 1 is strictly solo: if other players are on tile, step away immediately!
	if (here.players > 1)
		return "avance";

	if (here.resources[LINEMATE] >= 1 && inv[NOURRITURE] >= 9)
		return "incantation";

	if (inv[LINEMATE] >= 1)
	{
		if (here.resources[LINEMATE] < 1)
			return "pose " + Protocol::resourceName(LINEMATE);
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
	const t_tile_content &here = vision[0];
	const int *inv = ctx->getInventory();

	// Never linger on an occupied tile while foraging: step forward to avoid disturbing rituals
	if (here.players > 1)
		return "avance";

	if (here.resources[NOURRITURE] > 0 && inv[NOURRITURE] < 45)
		return "prend " + Protocol::resourceName(NOURRITURE);

	// In FORAGE, do not hoover stones from a tile if someone is currently leading or teammates are on the tile
	bool isTileIncantationReady = ElevationManager::isTileResourceComplete(ctx->getLevel(), here);
	if (!isTileIncantationReady && here.players <= 1)
	{
		for (size_t i = 0; i < STONE_COUNT; i++)
		{
			if (here.resources[STONES[i]] > 0)
				return "prend " + Protocol::resourceName(STONES[i]);
		}
	}

	if (inv[NOURRITURE] >= 14)
	{
		bool hasStones = ElevationManager::hasAllElevationStones(ctx->getLevel(), inv);
		if (!hasStones && ctx->getLevel() >= 4)
			hasStones = ElevationManager::hasEnoughToLead(ctx->getLevel(), inv);

		if (hasStones)
		{
			bool leaderActive = ctx->hasActiveLeaderForLevel(ctx->getLevel());
			int activeLeaderId = ctx->getActiveLeaderForLevel(ctx->getLevel());

			if (!leaderActive || ctx->getMyId() > activeLeaderId)
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
	}

	if (inv[NOURRITURE] < 12)
	{
		for (size_t i = 1; i < vision.size(); i++)
		{
			if (vision[i].resources[NOURRITURE] > 0)
				return SoundNavigator::stepTowardVisionTile(i, ctx);
		}
	}

	const s_elevation_req &req = Elevation::getRequirement(ctx->getLevel());

	// Prioritize stepping towards missing elevation stones
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
	// Level 1 is solo (always 1). For higher levels, count only confirmed matching-level followers!
	int sameLvlPlayers = (targetPlayers == 1) ? 1 : (1 + ctx->getConfirmedFollowersCount());
	// Strict requirement: EXACTLY targetPlayers on tile physically, ni plus ni moins!
	bool playersComplete = (sameLvlPlayers >= targetPlayers) && (here.players == targetPlayers);

	if (stonesComplete && playersComplete)
	{
		mzu::info("All requirements met on tile! Firing FULL, INCANT, and starting incantation!");
		ctx->transitionTo(ctx->getStateIncanting());
		ctx->setQueuedCommand("incantation");
		return "broadcast " + ctx->getMessenger().buildFull(ctx->getMyId(), ctx->getLevel());
	}

	// If ritual stones are ready, but extraneous players are present on tile, expulse them!
	if (stonesComplete && here.players > targetPlayers)
	{
		mzu::info("Extraneous players on tile (total=" + mzu::intToString(here.players) + ", needed=" + mzu::intToString(targetPlayers) + "); sending expulse!");
		ctx->setQueuedCommand("voir");
		return "expulse";
	}

	// 4. Timeout safety: only abort if stuck without progress for 150 ticks
	if (ctx->incrementGatherTimeout() > 150)
	{
		mzu::info("Gathering timed out (150 ticks). Returning to FORAGE.");
		ctx->resetGatherTimeout();
		ctx->transitionTo(ctx->getStateForage());
		ctx->setQueuedCommand("avance");
		return "broadcast " + ctx->getMessenger().buildAbort(ctx->getMyId(), ctx->getLevel());
	}

	// 5. Broadcast OFFER to guide teammates to this tile
	int slotsNeeded = targetPlayers - here.players;
	if (slotsNeeded < 0) slotsNeeded = 0;

	// If missing teammates to elevate and nobody has arrived after waiting, lay AT MOST 1 egg if team size < 8!
	if (slotsNeeded > 0 && inv[NOURRITURE] >= 16 && ctx->canLayEgg() && ctx->getGatherTimeout() > 50)
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

	const std::vector<t_tile_content> &vision = ctx->getVision();
	int dir = ctx->getTargetLeaderDirection();

	if (dir > 0)
	{
		if (ctx->hasVision() && !vision.empty() && vision[0].resources[NOURRITURE] > 0 && ctx->getInventory()[NOURRITURE] < 20)
			return "prend " + Protocol::resourceName(NOURRITURE);
		ctx->setTargetLeaderDirection(-1);
		return SoundNavigator::stepSoundDirection(dir, ctx);
	}
	else if (dir == 0 || ctx->getTargetLeaderDirection() == 0)
	{
		const std::vector<t_tile_content> &vision = ctx->getVision();
		const t_tile_content &here = vision[0];
		const int *inv = ctx->getInventory();

		// Timeout safety: if on tile for 20 ticks and ritual hasn't started, step off!
		if (ctx->incrementGatherWaitTicks() > 20)
		{
			mzu::info("Waited 20 ticks on leader tile with no incantation started. Stepping off to FORAGE!");
			ctx->resetGatherWaitTicks();
			ctx->transitionTo(ctx->getStateForage());
			ctx->setTargetLeaderId(-1);
			ctx->setTargetLeaderDirection(-1);
			return "avance";
		}

		if (here.resources[NOURRITURE] > 0 && inv[NOURRITURE] < 40)
			return "prend " + Protocol::resourceName(NOURRITURE);

		int targetPlayers = Elevation::getRequirement(ctx->getLevel()).players_needed;
		if (here.players > targetPlayers)
		{
			mzu::info("Too many players on leader tile (" + mzu::intToString(here.players) + " > " + mzu::intToString(targetPlayers) + "); stepping off!");
			ctx->transitionTo(ctx->getStateForage());
			ctx->setTargetLeaderId(-1);
			ctx->setTargetLeaderDirection(-1);
			return "avance";
		}

		if (inv[NOURRITURE] < 8)
		{
			mzu::info("Low food while waiting for Leader, returning to forage");
			ctx->transitionTo(ctx->getStateForage());
			ctx->setTargetLeaderId(-1);
			ctx->setTargetLeaderDirection(-1);
			return "voir";
		}

		// Periodically confirm to leader that we are on tile
		if (ctx->incrementBroadcastTimer() >= 2)
		{
			ctx->resetBroadcastTimer();
			return "broadcast " + ctx->getMessenger().buildHere(ctx->getMyId(), ctx->getLevel(), ctx->getTargetLeaderId());
		}

		// Deposit needed stones
		int missing[RESOURCE_COUNT];
		ElevationManager::calculateMissingStones(ctx->getLevel(), here, missing);

		e_resource toDeposit = ElevationManager::getStoneToDeposit(inv, missing);
		if (toDeposit != RESOURCE_COUNT)
			return "pose " + Protocol::resourceName(toDeposit);

		// If resources are complete, stay on tile waiting for incantation
		return "voir";
	}

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
