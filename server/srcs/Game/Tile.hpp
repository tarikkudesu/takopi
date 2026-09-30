#ifndef __TILE_HPP__
#define __TILE_HPP__

#include "../utilities/BasicString.hpp"

class Tile
{
	private:
		int								__x;
		int								__y;
		int								__resources[RESOURCE_COUNT];
		std::vector<int>				__playerIds;

	public:
		Tile();
		Tile(int x, int y);
		Tile(const Tile &copy);
		Tile	&operator=(const Tile &assign);
		~Tile();

		int								getX() const;
		int								getY() const;
		int								getResource(Resource type) const;
		const std::vector<int>			&getPlayerIds() const;

		void							addResource(Resource type, int count);
		void							removeResource(Resource type);
		bool							hasResource(Resource type) const;
		void							addPlayer(int playerId);
		void							removePlayer(int playerId);
		String							contentString() const;
};

#endif
