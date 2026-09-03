#ifndef __WORLD_HPP__
#define __WORLD_HPP__

#include "Elevation.hpp"

class World
{
	private:
		std::vector< std::vector<Tile> >		__map;
		int										__width;
		int										__height;

	public:
		World();
		World(const World &copy);
		World									&operator=(const World &assign);
		~World();

		int										getWidth() const;
		int										getHeight() const;
		int										wrapX(int x) const;
		int										wrapY(int y) const;
		void									populateResources();
		Tile									&tileAt(int x, int y);
		const Tile								&tileAt(int x, int y) const;
		void									init(int width, int height);

		int										broadcastDirection(int fromX, int fromY, int toX, int toY, e_direction facing) const;
		String									buildVisionString(const Player &player) const;
		void									display() const;
};

#endif
