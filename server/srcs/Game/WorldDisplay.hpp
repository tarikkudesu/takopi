#ifndef __WORLD_DISPLAY_HPP__
#define __WORLD_DISPLAY_HPP__

/* ************************************************************************** */
/*  TEMPORARY RAYLIB DISPLAY — delete this file to remove the GUI             */
/* ************************************************************************** */

class World;

void	initDisplay(int mapWidth, int mapHeight);
void	updateDisplay(const World &world);
void	closeDisplay();

#endif
