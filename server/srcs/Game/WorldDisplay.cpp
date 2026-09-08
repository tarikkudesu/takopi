/* ************************************************************************** */
/*  TEMPORARY RAYLIB DISPLAY — delete this file to remove the GUI             */
/* ************************************************************************** */

#include "World.hpp"
#include "WorldDisplay.hpp"

/* undef ANSI color macros from zappy.hpp — they collide with raylib's Color macros */
#undef RED
#undef GREEN
#undef BLUE
#undef YELLOW
#undef CYAN
#undef MAGENTA
#undef RESET

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#pragma GCC diagnostic pop

#define TILE_SIZE  80
#define GRID_PAD   40

static bool	sInitialized = false;

/* ---- resource colours --------------------------------------------------- */

static const e_resource RESOURCE_ORDER[RESOURCE_COUNT] = {
	NOURRITURE, LINEMATE, DERAUMERE, SIBUR,
	MENDIANE, PHIRAS, THYSTAME
};

static const Color RESOURCE_COLORS[RESOURCE_COUNT] = {
	GREEN,        // NOURRITURE
	SKYBLUE,      // LINEMATE
	BLUE,         // DERAUMERE
	ORANGE,       // SIBUR
	YELLOW,       // MENDIANE
	PINK,         // PHIRAS
	PURPLE        // THYSTAME
};

static const char *RESOURCE_NAMES[RESOURCE_COUNT] = {
	"Food", "Linemate", "Deraumere", "Sibur",
	"Mendiane", "Phiras", "Thystame"
};

/* ---- interface ---------------------------------------------------------- */

void initDisplay(int mapWidth, int mapHeight)
{
	if (sInitialized)
		return;
	int winW = mapWidth  * TILE_SIZE + GRID_PAD * 2;
	int winH = mapHeight * TILE_SIZE + GRID_PAD * 2 + 40;
	InitWindow(winW, winH, "Zappy — World View");
	SetTargetFPS(30);
	sInitialized = true;
}

void updateDisplay(const World &world)
{
	if (!sInitialized)
		return;
	if (WindowShouldClose())
		return;

	int w = world.getWidth();
	int h = world.getHeight();

	BeginDrawing();
	ClearBackground({30, 30, 30, 255});

	/* ---- draw grid ------------------------------------------------------ */
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			int px = GRID_PAD + x * TILE_SIZE;
			int py = GRID_PAD + y * TILE_SIZE;

			// tile background
			DrawRectangle(px, py, TILE_SIZE - 1, TILE_SIZE - 1, {50, 50, 50, 255});

			const Tile &tile = world.tileAt(x, y);

			// draw resource dots (small circles in a row inside the tile)
			int dotCount = 0;
			for (int r = 0; r < RESOURCE_COUNT && dotCount < 7; r++)
			{
				int qty = tile.getResource(RESOURCE_ORDER[r]);
				if (qty > 0)
				{
					int dx = 4 + dotCount * 4;
					int dy = TILE_SIZE - 6;
					DrawCircle(px + dx, py + dy, 2, RESOURCE_COLORS[r]);
					dotCount++;
				}
			}

			// draw players
			const std::vector<int> &pids = tile.getPlayerIds();
			if (!pids.empty())
			{
				int cx = px + TILE_SIZE / 2;
				int cy = py + TILE_SIZE / 2 - 2;
				DrawCircle(cx, cy, 8, MAGENTA);
				const char *countStr = TextFormat("%d", (int)pids.size());
				int tw = MeasureText(countStr, 10);
				DrawText(countStr, cx - tw / 2, cy - 5, 10, WHITE);
			}
		}
	}

	/* ---- grid lines ----------------------------------------------------- */
	Color lineCol = {70, 70, 70, 255};
	for (int x = 0; x <= w; x++)
		DrawLine(GRID_PAD + x * TILE_SIZE, GRID_PAD,
				 GRID_PAD + x * TILE_SIZE, GRID_PAD + h * TILE_SIZE, lineCol);
	for (int y = 0; y <= h; y++)
		DrawLine(GRID_PAD, GRID_PAD + y * TILE_SIZE,
				 GRID_PAD + w * TILE_SIZE, GRID_PAD + y * TILE_SIZE, lineCol);

	/* ---- axis labels ---------------------------------------------------- */
	for (int x = 0; x < w; x++)
	{
		const char *label = TextFormat("%d", x);
		int tw = MeasureText(label, 10);
		DrawText(label, GRID_PAD + x * TILE_SIZE + TILE_SIZE / 2 - tw / 2,
				 GRID_PAD - 14, 10, LIGHTGRAY);
	}
	for (int y = 0; y < h; y++)
	{
		const char *label = TextFormat("%d", y);
		DrawText(label, GRID_PAD - 18,
				 GRID_PAD + y * TILE_SIZE + TILE_SIZE / 2 - 5, 10, LIGHTGRAY);
	}

	/* ---- legend --------------------------------------------------------- */
	int legendY = GRID_PAD + h * TILE_SIZE + 10;
	int legendX = GRID_PAD;
	for (int r = 0; r < RESOURCE_COUNT; r++)
	{
		DrawCircle(legendX + 6, legendY + 6, 5, RESOURCE_COLORS[r]);
		DrawText(RESOURCE_NAMES[r], legendX + 16, legendY, 10, LIGHTGRAY);
		legendX += MeasureText(RESOURCE_NAMES[r], 10) + 28;
	}

	EndDrawing();
}

void closeDisplay()
{
	if (!sInitialized)
		return;
	CloseWindow();
	sInitialized = false;
}
