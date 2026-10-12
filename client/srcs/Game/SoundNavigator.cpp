#include "SoundNavigator.hpp"
#include "Strategy.hpp"
#include <cstdlib>
#include <cmath>

SoundNavigator::SoundNavigator()
{
}

SoundNavigator::~SoundNavigator()
{
}

String SoundNavigator::stepSoundDirection(int soundDirection, Strategy *ctx)
{
	switch (soundDirection)
	{
		case 1:
			return "avance";
		case 2:
			return "avance";
		case 3:
			if (ctx) ctx->setQueuedCommand("avance");
			return "gauche";
		case 4:
			if (ctx) ctx->setQueuedCommand("avance");
			return "gauche";
		case 5:
			// 180-degree turn: two droite turns + avance towards the sound
			if (ctx)
			{
				ctx->setQueuedCommand("droite");
				ctx->queueCommand("avance");
			}
			return "droite";
		case 6:
			if (ctx) ctx->setQueuedCommand("avance");
			return "droite";
		case 7:
			if (ctx) ctx->setQueuedCommand("avance");
			return "droite";
		case 8:
			return "avance";
		default:
			return "avance";
	}
}

String SoundNavigator::stepTowardVisionTile(size_t tileIndex, Strategy *ctx)
{
	int idx = static_cast<int>(tileIndex);
	if (idx <= 0)
		return "avance";

	// Vision indexing in Zappy FOV:
	// Level 1:
	// [1, 2, 3]  (ring 1: 1=left, 2=front, 3=right)
	// Level 2:
	// [1, 2, 3]  (ring 1)
	// [4, 5, 6, 7, 8] (ring 2: 4=leftmost, 6=front, 8=rightmost)
	// General rule for ring R:
	// Ring starts at R^2, front tile is R^2 + R.
	int ring = static_cast<int>(std::sqrt(static_cast<double>(idx)));
	while (ring * ring > idx) ring--;
	while ((ring + 1) * (ring + 1) <= idx) ring++;

	int frontIdx = ring * ring + ring;

	// If the tile is directly ahead, march forward!
	if (idx == frontIdx)
		return "avance";

	// Always queue "avance" after turning so the bot never oscillates on the same tile!
	if (idx < frontIdx)
	{
		if (ctx) ctx->setQueuedCommand("avance");
		return "gauche";
	}
	else
	{
		if (ctx) ctx->setQueuedCommand("avance");
		return "droite";
	}
}

String SoundNavigator::wander(Strategy *ctx)
{
	int r = rand() % 3;
	if (r == 0)
	{
		if (ctx) ctx->setQueuedCommand("avance");
		return "droite";
	}
	if (r == 1)
	{
		if (ctx) ctx->setQueuedCommand("avance");
		return "gauche";
	}
	return "avance";
}
