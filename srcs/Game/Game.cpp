#include "Game.hpp"
#include "../ServerManager/ServerManager.hpp"
#include "../ServerManager/Core.hpp"

GameConfig::GameConfig() : port(0),
						   portSet(false),
						   width(0),
						   height(0),
						   configPath(DEFAULT_CONFIG_FILE_PATH)
{
}

Game::Game(const GameConfig &config) : __config(config)
{
}
Game::~Game()
{
}

void Game::start()
{
	ServerManager manager(__config);
	manager.setUpZappy();
}

/*****************************************************************************
 *								ARGUMENT PARSING							 *
 *****************************************************************************/

static bool isNumber(const String &str)
{
	if (str.empty())
		return false;
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str.at(i) < '0' || str.at(i) > '9')
			return false;
	}
	return true;
}

static int toInt(const String &str)
{
	long value = wsu::stringToInt(str);
	if (value < INT_MIN || value > INT_MAX)
		throw std::runtime_error("invalid numeric value: " + str);
	return static_cast<int>(value);
}

static t_svec collectValues(char **av, int ac, int &i, const String &flag)
{
	t_svec values;

	i++;
	while (i < ac && String(av[i]).find('-') != 0)
	{
		values.push_back(String(av[i]));
		i++;
	}
	if (values.empty())
		throw std::runtime_error(flag + ": missing value(s)");
	i--;
	return values;
}

static void parsePort(GameConfig &config, const String &flag, char **av, int ac, int &i)
{
	if (++i >= ac || !isNumber(String(av[i])))
		throw std::runtime_error(flag + ": expected a positive number");
	config.port = toInt(String(av[i]));
	config.portSet = true;
	if (config.port < 1 || config.port > 65535)
		throw std::runtime_error(flag + ": port out of range [1, 65535]");
}

static void parseDimension(int &dimension, const String &flag, char **av, int ac, int &i)
{
	if (++i >= ac || !isNumber(String(av[i])))
		throw std::runtime_error(flag + ": expected a positive number");
	dimension = toInt(String(av[i]));
}

static void applyLogLevels(const t_svec &levels)
{
	wsu::logs(levels);
}

GameConfig Game::parseArgs(int ac, char **av)
{
	GameConfig config;

	for (int i = 1; i < ac; i++)
	{
		String arg(av[i]);
		if (arg == "-p" || arg == "--port")
			parsePort(config, arg, av, ac, i);
		else if (arg == "-x" || arg == "--width")
			parseDimension(config.width, arg, av, ac, i);
		else if (arg == "-y" || arg == "--height")
			parseDimension(config.height, arg, av, ac, i);
		else if (arg == "-n" || arg == "--names")
		{
			t_svec names = collectValues(av, ac, i, arg);
			config.teams.insert(config.teams.end(), names.begin(), names.end());
		}
		else if (arg == "-c" || arg == "--config")
		{
			if (++i >= ac)
				throw std::runtime_error(arg + ": expected a file path");
			config.configPath = String(av[i]);
		}
		else if (arg == "-l" || arg == "--logs")
			applyLogLevels(collectValues(av, ac, i, arg));
		else
			throw std::runtime_error("unknown option: \"" + arg + "\"" + USAGE);
	}
	return config;
}
