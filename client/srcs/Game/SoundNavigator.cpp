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
	// Ring R starts at R^2, front tile is R^2 + R.
	int ring = static_cast<int>(std::sqrt(static_cast<double>(idx)));
	while (ring * ring > idx) ring--;
	while ((ring + 1) * (ring + 1) <= idx) ring++;

	int frontIdx = ring * ring + ring;

	// In ring 1: [1=left, 2=front, 3=right]
	if (ring == 1)
	{
		if (idx == 1)
		{
			if (ctx) ctx->setQueuedCommand("avance");
			return "gauche";
		}
		if (idx == 3)
		{
			if (ctx) ctx->setQueuedCommand("avance");
			return "droite";
		}
		return "avance";
	}

	// For rings >= 2, advance directly if tile is in the forward corridor
	int offset = std::abs(idx - frontIdx);
	if (offset <= ring / 2)
		return "avance";

	// Otherwise, step towards the side:
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
	int r = rand() % 5;
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
