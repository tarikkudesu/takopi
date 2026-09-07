#include "ServerAdmin.hpp"
#include "ServerGame.hpp"
#include "ServerGui.hpp"
#include "ServerManager.hpp"

ServerManager::ServerManager(const String &configutation_file) : __config(configutation_file)
{
	mzu::debug("ServerManager para constructor");
	mzu::info("configuration file: " + configutation_file);
}

ServerManager::~ServerManager()
{
	for (t_serVect::iterator it = __serverTemplates.begin(); it != __serverTemplates.end(); it++)
		delete *it;
	this->__serverTemplates.clear();
	Core::clear();
	mzu::debug("ServerManager destructor");
}

/*************************************************************************
 *							   SERVER PARSER							 *
 *************************************************************************/

void ServerManager::readFile()
{
	std::fstream fS;
	String line;

	fS.open(__config.c_str());
	if (!fS.is_open())
		throw std::runtime_error("coudln't open configuration file: " + __config);
	do
	{
		std::getline(fS, line, '\n');
		if (fS.fail())
			break;
		size_t pos = line.find("# ");
		if (pos != std::string::npos)
			line = line.substr(0, pos);
		if (line.empty() || String::npos == line.find_first_not_of(" \t\n\r\v\f"))
			continue;
		this->__lines.append(line);
		this->__lines.append(" ");
		line.clear();
		if (fS.eof())
			break;
	} while (true);
	fS.close();
}
void ServerManager::firstCheck()
{
	if (__lines.empty() || String::npos == __lines.find_first_not_of(" \t\n\r\v\f"))
		throw std::runtime_error("empty configuration file");
	if (String::npos == __lines.find_first_of("{}"))
		throw std::runtime_error("invalid configuration file");
	if (String::npos != __lines.find_first_not_of(PRINTABLE))
		throw std::runtime_error("unknown characters");
}
void ServerManager::checkBraces()
{
	size_t end = 0;
	size_t tracker = 0;
	do
	{
		if (end >= this->__lines.length())
			break;
		if (this->__lines.at(end) == '}')
			tracker--;
		if (this->__lines.at(end) == '{')
			tracker++;
		end++;
	} while (true);
	if (tracker != 0)
		throw std::runtime_error("unclosed curly braces");
}
void ServerManager::reduceSpaces()
{
	std::string result;
	bool inSpace = false;

	for (size_t i = 0; i < __lines.length(); i++)
	{
		if (std::isspace(__lines.at(i)))
		{
			if (!inSpace)
			{
				result += ' ';
				inSpace = true;
			}
		}
		else
		{
			result += __lines.at(i);
			inSpace = false;
		}
	}
	this->__lines.clear();
	this->__lines.append(result);
}
String ServerManager::checkOuterscope(String outerScope)
{
	mzu::trimSpaces(outerScope);
	if (outerScope != "game" && outerScope != "admin" && outerScope != "gui")
		throw std::runtime_error("invalid configuration file: unknown server type \"" + outerScope + "\"");
	if (__lines.find_first_of("{}") == String::npos)
		throw std::runtime_error("invalid configuration file ");
	return outerScope;
}
void ServerManager::setUpServer(size_t start)
{
	size_t end = start + 1;
	size_t tracker = 1;

	String type = checkOuterscope(String(this->__lines.begin(), this->__lines.begin() + start));
	do
	{
		if (end >= this->__lines.length())
			break;
		if (this->__lines.at(end) == '}')
			tracker--;
		if (this->__lines.at(end) == '{')
			tracker++;
		end++;
		if (tracker == 0)
			break;
	} while (true);
	if (tracker != 0)
		throw std::runtime_error("unclosed curly braces");
	String serverConfig(this->__lines.begin() + start, this->__lines.begin() + end);
	this->__lines.erase(0, end);
	Server *server = NULL;
	try
	{
		if (type == "game")
			server = new ServerGame(serverConfig);
		else if (type == "admin")
			server = new ServerAdmin(serverConfig);
		else if (type == "gui")
			server = new ServerGui(serverConfig);
		else
			throw std::runtime_error("invalid server type");
	}
	catch (std::exception &e)
	{
		(void)e;
		delete server;
		throw;
	}
	mzu::debug(type + " block parsed");
	__serverTemplates.push_back(server);
}
void ServerManager::setUpServers()
{
	do
	{
		size_t pos = this->__lines.find("{");
		if (pos == String::npos && __lines.find_first_not_of(" \t\n\r\v\f") != String::npos)
			throw std::runtime_error("invalid configuration file");
		else if (pos == String::npos)
			break;
		setUpServer(pos);
	} while (!this->__lines.empty());
	mzu::info("syntax check: OK");
}

void ServerManager::validateServerCardinality()
{
	size_t counts[3] = {0, 0, 0};

	for (t_serVect::iterator it = __serverTemplates.begin(); it != __serverTemplates.end(); it++)
		counts[(*it)->getType()]++;
	if (counts[GAME] != 1)
		throw std::runtime_error("configuration requires exactly one game server");
	if (counts[ADMIN] != 1)
		throw std::runtime_error("configuration requires exactly one admin server");
	if (counts[GUI] != 1)
		throw std::runtime_error("configuration requires exactly one gui server");
}

void ServerManager::initServers()
{
	std::vector<int> 	portsTaken;

	for (t_serVect::iterator it = __serverTemplates.begin(); it != __serverTemplates.end(); it++)
	{
		Server *tmp = *it;
		for (std::vector<int>::iterator pt = portsTaken.begin(); pt != portsTaken.end(); pt++)
		{
			if (*pt == tmp->getServerPort())
				throw std::runtime_error(tmp->serverIdentity() + ": port conflict, " + mzu::intToString(*pt) + " is already in use");
		}
		portsTaken.push_back(tmp->getServerPort());
		try
		{
			tmp->setup();
			Core::addServer(tmp);
			*it = NULL;
		}
		catch (std::exception &e)
		{
			delete tmp;
			*it = NULL;
			mzu::error(e.what());
			throw;
		}
	}
	__serverTemplates.clear();
}

/*************************************************************************
 *                             SERVER LAUNCHER                           *
 *************************************************************************/

bool ServerManager::setUpZappy()
{
	try
	{
		readFile();
		firstCheck();
		reduceSpaces();
		checkBraces();
		setUpServers();
		validateServerCardinality();
		initServers();
		Core::logServers();
		Core::mainLoop();
	}
	catch (std::exception &e)
	{
		mzu::terr(e.what());
		return false;
	}
	return true;
}
