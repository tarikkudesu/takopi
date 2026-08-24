#include "GameServer.hpp"

GameServer::GameServer(String line) : __width(-1),
									   __height(-1)
{
	mzu::debug("GameServer constructor");
	this->__type = "game";
	parseBlock(line);
	if (!this->__portSet)
		throw std::runtime_error("game server: missing \"port\" directive");
	if (this->__width < 1 || this->__height < 1)
		throw std::runtime_error("game server: missing map dimension directive (\"width\"/\"height\")");
	if (this->__teams.empty())
		throw std::runtime_error("game server: missing \"teams\" directive");
}
GameServer::GameServer(const GameServer &copy) : Server(copy),
												 __width(copy.__width),
												 __height(copy.__height),
												 __teams(copy.__teams)
{
	mzu::debug("GameServer copy constructor");
}
GameServer &GameServer::operator=(const GameServer &assign)
{
	mzu::debug("GameServer copy assignement operator");
	if (this != &assign)
	{
		Server::operator=(assign);
		this->__width = assign.__width;
		this->__height = assign.__height;
		this->__teams = assign.__teams;
	}
	return *this;
}
GameServer::~GameServer()
{
	this->__teams.clear();
	mzu::debug("GameServer destructor");
}

/****************************************************************************
 *								 MINI METHODS								*
 ****************************************************************************/

int GameServer::getMapWidth() const
{
	return this->__width;
}
int GameServer::getMapHeight() const
{
	return this->__height;
}
const t_svec &GameServer::getTeams() const
{
	return this->__teams;
}

/****************************************************************************
 *						  PROCCESING DIRECTIVES							  *
 ****************************************************************************/

void GameServer::proccessWidthToken(t_svec &tokens)
{
	if (this->__width != -1)
		throw std::runtime_error(tokens.at(0) + " directive is duplicate");
	if (tokens.size() == 1)
		throw std::runtime_error(tokens.at(0) + ": no width value");
	if (tokens.size() > 2)
		throw std::runtime_error(tokens.at(0) + ": multiple width values");
	if (String::npos != tokens.at(1).find_first_not_of("0123456789"))
		throw std::runtime_error(tokens.at(0) + ": invalid width: not a number");
	size_t width = mzu::stringToInt(tokens.at(1));
	if (width < 1 || width > 10000)
		throw std::runtime_error(tokens.at(0) + ": invalid width: out of range");
	this->__width = static_cast<int>(width);
}

void GameServer::proccessHeightToken(t_svec &tokens)
{
	if (this->__height != -1)
		throw std::runtime_error(tokens.at(0) + " directive is duplicate");
	if (tokens.size() == 1)
		throw std::runtime_error(tokens.at(0) + ": no height value");
	if (tokens.size() > 2)
		throw std::runtime_error(tokens.at(0) + ": multiple height values");
	if (String::npos != tokens.at(1).find_first_not_of("0123456789"))
		throw std::runtime_error(tokens.at(0) + ": invalid height: not a number");
	size_t height = mzu::stringToInt(tokens.at(1));
	if (height < 1 || height > 10000)
		throw std::runtime_error(tokens.at(0) + ": invalid height: out of range");
	this->__height = static_cast<int>(height);
}

void GameServer::proccessTeamsToken(t_svec &tokens)
{
	for (t_svec::iterator it = tokens.begin() + 1; it != tokens.end(); it++)
	{
		if (std::find(this->__teams.begin(), this->__teams.end(), *it) != this->__teams.end())
			throw std::runtime_error(tokens.at(0) + ": duplicate team name: " + *it);
		this->__teams.push_back(*it);
	}
	if (this->__teams.empty())
		throw std::runtime_error(tokens.at(0) + ": no teams values");
}

void GameServer::proccessToken(t_svec &tokens)
{
	String key = tokens.at(0);
	if (key != "port" &&
		key != "host" &&
		key != "width" &&
		key != "height" &&
		key != "teams")
		throw std::runtime_error(key + ": unknown directive");
	if (key == "port")
		proccessPortToken(tokens);
	else if (key == "host")
		proccessHostToken(tokens);
	else if (key == "width")
		proccessWidthToken(tokens);
	else if (key == "height")
		proccessHeightToken(tokens);
	else
		proccessTeamsToken(tokens);
}
