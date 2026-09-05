#include "World.hpp"
#include "WorldDisplay.hpp"
#include "Player.hpp"
#include <cstdlib>
#include <cmath>

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

World::World() : __width(0), __height(0)
{
}

World::World(const World &copy)
{
	*this = copy;
}

World &World::operator=(const World &assign)
{
	if (this != &assign)
	{
		__width = assign.__width;
		__height = assign.__height;
		__map = assign.__map;
	}
	return *this;
}

World::~World()
{
	closeDisplay();
}

/*************************************************************************
 *                            ACCESSORS                                  *
 *************************************************************************/

int World::getWidth() const
{
	return __width;
}

int World::getHeight() const
{
	return __height;
}

int World::wrapX(int x) const
{
	return ((x % __width) + __width) % __width;
}

int World::wrapY(int y) const
{
	return ((y % __height) + __height) % __height;
}

Tile &World::tileAt(int x, int y)
{
	return __map[wrapY(y)][wrapX(x)];
}

const Tile &World::tileAt(int x, int y) const
{
	return __map[wrapY(y)][wrapX(x)];
}

/*************************************************************************
 *                         INITIALIZATION                                *
 *************************************************************************/

void World::init(int width, int height)
{
	__width = width;
	__height = height;
	__map.resize(height);
	for (int y = 0; y < height; y++)
	{
		__map[y].resize(width);
		for (int x = 0; x < width; x++)
			__map[y][x] = Tile(x, y);
	}
	initDisplay(width, height);
}

void World::resize(int width, int height)
{
	if (width < 1 || width > MAP_MAX_SIZE || height < 1 || height > MAP_MAX_SIZE)
		throw std::runtime_error("map dimensions must be between 1 and " + MAP_MAX_SIZE);

	std::vector< std::vector<Tile> > map(height);
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			Tile tile(x, y);
			if (y < __height && x < __width)
			{
				for (int r = 0; r < RESOURCE_COUNT; r++)
					tile.addResource(static_cast<e_resource>(r), __map[y][x].getResource(static_cast<e_resource>(r)));
			}
			map[y].push_back(tile);
		}
	}
	for (int y = 0; y < __height; y++)
	{
		for (int x = 0; x < __width; x++)
		{
			const std::vector<int> &ids = __map[y][x].getPlayerIds();
			for (size_t i = 0; i < ids.size(); i++)
				map[y % height][x % width].addPlayer(ids[i]);
		}
	}
	__map.swap(map);
	__width = width;
	__height = height;
	populateResources();
}

void World::populateResources()
{
	static const double densities[RESOURCE_COUNT] = {
		0.5,
		0.3,
		0.15,
		0.1,
		0.1,
		0.08,
		0.05
	};
	for (int y = 0; y < __height; y++)
	{
		for (int x = 0; x < __width; x++)
		{
			for (int r = 0; r < RESOURCE_COUNT; r++)
			{
				double roll = static_cast<double>(rand()) / RAND_MAX;
				if (roll < densities[r])
					__map[y][x].addResource(static_cast<e_resource>(r), 1);
			}
		}
	}
}

/*************************************************************************
 *                         FIELD OF VIEW                                 *
 *************************************************************************/

String World::buildVisionString(const Player &player) const
{
	static const int fwd_dx[] = {0, 1, 0, -1};
	static const int fwd_dy[] = {-1, 0, 1, 0};
	static const int rgt_dx[] = {1, 0, -1, 0};
	static const int rgt_dy[] = {0, 1, 0, -1};

	int dir = player.getDirection();
	int px = player.getX();
	int py = player.getY();
	int level = player.getLevel();

	String result = "{";
	bool first = true;

	for (int i = 0; i <= level; i++)
	{
		for (int s = -i; s <= i; s++)
		{
			if (!first)
				result += ",";
			first = false;
			int x = px + fwd_dx[dir] * i + rgt_dx[dir] * s;
			int y = py + fwd_dy[dir] * i + rgt_dy[dir] * s;
			result += " " + tileAt(x, y).contentString();
		}
	}
	result += "}";
	return result;
}

/*************************************************************************
 *                      BROADCAST DIRECTION                              *
 *************************************************************************/

int World::broadcastDirection(int fromX, int fromY, int toX, int toY, e_direction facing) const
{
	int dx = fromX - toX;
	int dy = fromY - toY;

	if (dx > __width / 2)
		dx -= __width;
	if (dx < -__width / 2)
		dx += __width;
	if (dy > __height / 2)
		dy -= __height;
	if (dy < -__height / 2)
		dy += __height;

	if (dx == 0 && dy == 0)
		return 0;

	double angle = atan2(static_cast<double>(-dy), static_cast<double>(dx));
	double degrees = angle * 180.0 / M_PI;
	if (degrees < 0)
		degrees += 360.0;

	double facingAngle;
	switch (facing)
	{
		case NORTH:	facingAngle = 90.0; break;
		case EAST:	facingAngle = 0.0; break;
		case SOUTH:	facingAngle = 270.0; break;
		case WEST:	facingAngle = 180.0; break;
		default:	facingAngle = 0.0; break;
	}

	double relative = degrees - facingAngle;
	if (relative < 0)
		relative += 360.0;

	relative += 22.5;
	if (relative >= 360.0)
		relative -= 360.0;

	int square = static_cast<int>(relative / 45.0) + 1;
	if (square > 8)
		square = 1;
	return square;
}

/*************************************************************************
 *                    TEMPORARY RAYLIB DISPLAY                           *
 *************************************************************************/

void World::display() const
{
	updateDisplay(*this);
}
