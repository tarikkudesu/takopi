#include "ServerGame.hpp"
#include "../Game/Game.hpp"

ServerGame::ServerGame(String line) : 	Server(GAME),
										__width(-1),
									   	__height(-1),
									   	__timeUnit(-1),
									   	__clientsPerTeam(-1)
{
	mzu::debug("ServerGame constructor");
	parseBlock(line);
	if (!this->__portSet)
		throw std::runtime_error("game server: missing \"port\" directive");
	if (this->__width < 1 || this->__height < 1)
		throw std::runtime_error("game server: missing map dimension directive (\"width\"/\"height\")");
	if (this->__teams.empty())
		throw std::runtime_error("game server: missing \"teams\" directive");
	if (this->__clientsPerTeam < 1)
		throw std::runtime_error("game server: missing \"clients_per_team\" directive");
	if (this->__timeUnit < 1)
		this->__timeUnit = 100;
}
ServerGame::ServerGame(const ServerGame &copy) : Server(copy),
												__teams(copy.__teams),
												 __width(copy.__width),
												 __height(copy.__height),
												 __timeUnit(copy.__timeUnit),
												 __clientsPerTeam(copy.__clientsPerTeam)
{
	mzu::debug("ServerGame copy constructor");
}
ServerGame &ServerGame::operator=(const ServerGame &assign)
{
	mzu::debug("ServerGame copy assignement operator");
	if (this != &assign)
	{
		Server::operator=(assign);
		this->__teams = assign.__teams;
		this->__width = assign.__width;
		this->__height = assign.__height;
		this->__timeUnit = assign.__timeUnit;
		this->__clientsPerTeam = assign.__clientsPerTeam;
	}
	return *this;
}
ServerGame::~ServerGame()
{
	delete __game;
	this->__teams.clear();
	mzu::debug("ServerGame destructor");
}

/****************************************************************************
 *								 MINI METHODS								*
 ****************************************************************************/

int ServerGame::getMapWidth() const
{
	return this->__width;
}
int ServerGame::getMapHeight() const
{
	return this->__height;
}
int ServerGame::getTimeUnit() const
{
	return this->__timeUnit;
}
int ServerGame::getClientsPerTeam() const
{
	return this->__clientsPerTeam;
}
const t_svec &ServerGame::getTeams() const
{
	return this->__teams;
}
Game *ServerGame::getGame()
{
	return this->__game;
}

/****************************************************************************
 *						  PROCCESING DIRECTIVES								  *
 ****************************************************************************/

void ServerGame::proccessWidthToken(t_svec &tokens)
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

void ServerGame::proccessHeightToken(t_svec &tokens)
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

void ServerGame::proccessTeamsToken(t_svec &tokens)
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

void ServerGame::proccessTimeToken(t_svec &tokens)
{
	if (this->__timeUnit != -1)
		throw std::runtime_error(tokens.at(0) + " directive is duplicate");
	if (tokens.size() == 1)
		throw std::runtime_error(tokens.at(0) + ": no time value");
	if (tokens.size() > 2)
		throw std::runtime_error(tokens.at(0) + ": multiple time values");
	if (String::npos != tokens.at(1).find_first_not_of("0123456789"))
		throw std::runtime_error(tokens.at(0) + ": invalid time: not a number");
	long t = mzu::stringToInt(tokens.at(1));
	if (t < 1 || t > 10000)
		throw std::runtime_error(tokens.at(0) + ": invalid time: out of range");
	this->__timeUnit = static_cast<int>(t);
}

void ServerGame::proccessClientsToken(t_svec &tokens)
{
	if (this->__clientsPerTeam != -1)
		throw std::runtime_error(tokens.at(0) + " directive is duplicate");
	if (tokens.size() == 1)
		throw std::runtime_error(tokens.at(0) + ": no value");
	if (tokens.size() > 2)
		throw std::runtime_error(tokens.at(0) + ": multiple values");
	if (String::npos != tokens.at(1).find_first_not_of("0123456789"))
		throw std::runtime_error(tokens.at(0) + ": invalid clients_per_team: not a number");
	long c = mzu::stringToInt(tokens.at(1));
	if (c < 1 || c > 1000)
		throw std::runtime_error(tokens.at(0) + ": invalid clients_per_team: out of range");
	this->__clientsPerTeam = static_cast<int>(c);
}

void ServerGame::proccessToken(t_svec &tokens)
{
	String key = tokens.at(0);
	if (key != "port" &&
		key != "host" &&
		key != "width" &&
		key != "height" &&
		key != "teams" &&
		key != "time" &&
		key != "clients_per_team")
		throw std::runtime_error(key + ": unknown directive");
	if (key == "port")
		proccessPortToken(tokens);
	else if (key == "host")
		proccessHostToken(tokens);
	else if (key == "width")
		proccessWidthToken(tokens);
	else if (key == "height")
		proccessHeightToken(tokens);
	else if (key == "teams")
		proccessTeamsToken(tokens);
	else if (key == "time")
		proccessTimeToken(tokens);
	else
		proccessClientsToken(tokens);
}

/****************************************************************************
 *							GAME INITIALIZATION								*
 ****************************************************************************/

void ServerGame::initGame()
{
	if (__game)
		return;
	__game = new Game();
	__game->init(__width, __height, __teams, __timeUnit, __clientsPerTeam);
}
