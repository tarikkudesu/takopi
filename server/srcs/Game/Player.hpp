#ifndef __PLAYER_HPP__
#define __PLAYER_HPP__

#include "../utilities/BasicString.hpp"

/*************************************************************************
 *                           TRANTORIAN                                  *
 *************************************************************************/

class Player
{
	private:
		int								__id;
		int								__x;
		int								__y;
		int								__level;
		int								__teamIndex;
		e_direction						__direction;
		e_player_state					__state;
		int								__inventory[RESOURCE_COUNT];
		long							__nextFoodTick;

	public:
		Player();
		Player(int id, int x, int y, int teamIndex, long spawnTick);
		Player(const Player &copy);
		Player							&operator=(const Player &assign);
		~Player();

		int								getId() const;
		int								getX() const;
		int								getY() const;
		int								getLevel() const;
		int								getTeamIndex() const;
		e_direction						getDirection() const;
		int								getInventory(e_resource type) const;
		long							getNextFoodTick() const;

		void							setPosition(int x, int y);
		void							setLevel(int level);
		void							setState(e_player_state state);
		void							setDirection(e_direction dir);
		void							setNextFoodTick(long tick);

		void							moveForward(int mapW, int mapH);
		void							moveInDirection(e_direction dir, int mapW, int mapH);
		void							turnLeft();
		void							turnRight();
		void							addToInventory(e_resource type, int count);
		bool							removeFromInventory(e_resource type);
		String							inventoryString() const;
		bool							isAlive() const;
};

#endif
