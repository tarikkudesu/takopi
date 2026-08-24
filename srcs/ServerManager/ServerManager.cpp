#include "ServerManager.hpp"

ServerManager::ServerManager(const GameConfig &config) : __config(config)
{
	wsu::debug("ServerManager constructor");
}

ServerManager::~ServerManager()
{
	Core::clear();
	wsu::debug("ServerManager destructor");
}

/*************************************************************************
 *						   CONFIG FILE FALLBACK						     *
 *************************************************************************/

void ServerManager::parseConfigFile(const String &path)
{
	std::fstream fs;
	String line;

	fs.open(path.c_str());
	if (!fs.is_open())
		throw std::runtime_error("couldn't open configuration file: " + path);
	while (std::getline(fs, line, '\n'))
	{
		t_svec tokens = wsu::splitBySpaces(line);
		if (tokens.empty())
			continue;
		if (tokens.at(0) == "port" && tokens.size() == 2 && !__config.portSet)
			__config.port = wsu::stringToInt(tokens.at(1)), __config.portSet = true;
		else if (tokens.at(0) == "width" && tokens.size() == 2 && !__config.width)
			__config.width = wsu::stringToInt(tokens.at(1));
		else if (tokens.at(0) == "height" && tokens.size() == 2 && !__config.height)
			__config.height = wsu::stringToInt(tokens.at(1));
		else if (tokens.at(0) == "team" && tokens.size() == 2)
			__config.teams.push_back(tokens.at(1));
		else
			wsu::warn("ignoring invalid configuration line: \"" + line + "\"");
	}
	fs.close();
}

void ServerManager::applyFileFallback()
{
	std::fstream probe;

	if (__config.portSet && !__config.teams.empty())
		return;
	probe.open(__config.configPath.c_str());
	if (!probe.is_open())
	{
		if (!__config.portSet)
			throw std::runtime_error("no port provided (use -p) and no configuration file found: " + __config.configPath);
		return;
	}
	probe.close();
	wsu::info("applying configuration file fallback: " + __config.configPath);
	parseConfigFile(__config.configPath);
}

/*************************************************************************
 *								SERVER CONTROL							 *
 *************************************************************************/

void ServerManager::launch()
{
	Server *server = new Server(__config.port, DEFAULT_HOST);
	try
	{
		server->setup();
		Core::addServer(server);
	}
	catch (const std::exception &e)
	{
		delete server;
		throw e;
	}
	Core::logServers();
	Core::mainLoop();
}

void ServerManager::setUpZappy()
{
	try
	{
		applyFileFallback();
		launch();
	}
	catch (const std::exception &e)
	{
		wsu::terr(e.what());
	}
}
