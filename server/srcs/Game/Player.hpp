#ifndef __PLAYER_HPP__
#define __PLAYER_HPP__

#include "../utilities/BasicString.hpp"

class Player
{
	private:
		int								__id;
		int								__x;
		int								__y;
		int								__level;
		PlayerState						__state;
		Direction						__direction;
		int								__teamIndex;
		long							__nextFoodTick;
		int								__inventory[RESOURCE_COUNT];

	public:
		Player();
		Player(int id, int x, int y, int teamIndex, long spawnTick);
		Player(const Player &copy);
		Player							&operator=(const Player &assign);
		~Player();

		int								getX() const;
		int								getY() const;
		int								getId() const;
		int								getLevel() const;
		int								getTeamIndex() const;
		Direction						getDirection() const;
		long							getNextFoodTick() const;
		int								getInventory(Resource type) const;

		void							setLevel(int level);
		void							setPosition(int x, int y);
		void							setNextFoodTick(long tick);
		void							setDirection(Direction dir);
		void							setState(PlayerState state);

		void							turnLeft();
		void							turnRight();
		bool							isAlive() const;
		String							inventoryString() const;
		void							moveForward(int mapW, int mapH);
		bool							removeFromInventory(Resource type);
		void							addToInventory(Resource type, int count);
		void							moveInDirection(Direction dir, int mapW, int mapH);
};

#endif
