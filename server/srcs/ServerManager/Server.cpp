#include "Server.hpp"

Server::Server(e_type type) : 	__sd(-1),
				   				__port(-1),
				   				__host(DEFAULT_HOST),
				   				__type(type),
				   				__portSet(false)
{
	mzu::debug("Server default constructor");
}
Server::~Server()
{
	if (__sd != -1)
		close(__sd);
	mzu::debug("Server destructor");
}

/****************************************************************************
 *								 MINI METHODS								*
 ****************************************************************************/

int Server::getServerSocket() const
{
	return this->__sd;
}
int Server::getServerPort() const
{
	return this->__port;
}
void Server::setPort(int port)
{
	this->__port = port;
}
const String &Server::getServerHost() const
{
	return this->__host;
}
const e_type &Server::getType() const
{
	return this->__type;
}
String Server::serverIdentity() const
{
	return this->__host + ":" + mzu::intToString(this->__port);
}

/****************************************************************************
 *								SOCKET SETUP								*
 ****************************************************************************/

void Server::setup()
{
	struct sockaddr_in addr;
	int ra = 1, rp = 1;

	this->__sd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->__sd == -1)
		throw std::runtime_error(serverIdentity() + ": non functional: failed to create socket ");
	try
	{
		if (-1 == setsockopt(__sd, SOL_SOCKET, SO_REUSEADDR, (void *)&ra, sizeof(ra)))
			throw std::runtime_error(serverIdentity() + ": non functional: failed to make reusable address");
		if (-1 == setsockopt(__sd, SOL_SOCKET, SO_REUSEPORT, (void *)&rp, sizeof(rp)))
			throw std::runtime_error(serverIdentity() + ": non functional: failed to make reusable port");
		addr.sin_family = AF_INET;
		{
			struct addrinfo hint;
			struct addrinfo *result = NULL;
			mzu::bzero(&hint, sizeof(hint));
			hint.ai_family = AF_INET;
			hint.ai_socktype = SOCK_STREAM;
			int status = getaddrinfo(this->__host.c_str(), NULL, &hint, &result);
			if (status == 0)
			{
				struct sockaddr_in *add = (struct sockaddr_in *)result->ai_addr;
				addr.sin_addr.s_addr = add->sin_addr.s_addr;
				freeaddrinfo(result);
			}
			else
			{
				throw std::runtime_error(serverIdentity() + " non functional: couldn't resolve server host name: " + this->__host);
			}
		}
		addr.sin_port = htons(this->__port);
		if (-1 == bind(this->__sd, (struct sockaddr *)&addr, sizeof(addr)))
			throw std::runtime_error(serverIdentity() + " non functional: failed to bind socket");
		if (-1 == listen(this->__sd, SOMAXCONN))
			throw std::runtime_error(serverIdentity() + " non functional: failed to listen for connections");
		furtherSetup();
	}
	catch (std::runtime_error &e)
	{
		(void)e;
		close(this->__sd);
		this->__sd = -1;
		throw;
	}
}

/****************************************************************************
 *						  PROCCESING DIRECTIVES							  *
 ****************************************************************************/

void Server::parseBlock(String line)
{
	mzu::trimSpaces(line);
	if (!line.empty())
		line = line.substr(1, line.length() - 2);
	mzu::trimSpaces(line);
	parseServerDirectives(line);
	proccessServerDirectives();
	this->__directives.clear();
}

void Server::addServerDirective(String &line, size_t end)
{
	String directive = String(line.begin(), line.begin() + end);
	mzu::trimSpaces(directive);
	if (directive.empty())
		throw std::runtime_error("empty directive");
	this->__directives.push_back(directive);
	line.erase(0, end + 1);
	mzu::debug("directive: " + directive);
}

void Server::parseServerDirectives(String line)
{
	do
	{
		size_t pos = line.find_first_of(";{");
		if (pos == String::npos && line.find_first_not_of(" \t\n\r\v\f") != String::npos)
			throw std::runtime_error("invalid block: expected \";\" at the end of each directive");
		if (pos == String::npos)
			break;
		if (line.at(pos) == ';')
			addServerDirective(line, pos);
		else
			throw std::runtime_error("invalid block: unexpected nested block");
	} while (true);
}

void Server::proccessServerDirectives()
{
	for (t_svec::iterator it = this->__directives.begin(); it != this->__directives.end(); it++)
	{
		t_svec tokens = mzu::splitBySpaces(*it);
		if (!tokens.empty())
			proccessToken(tokens);
	}
}

void Server::proccessPortToken(t_svec &tokens)
{
	if (this->__portSet)
		throw std::runtime_error(tokens.at(0) + " directive is duplicate");
	if (tokens.size() == 1)
		throw std::runtime_error(tokens.at(0) + ": no port value");
	if (tokens.size() > 2)
		throw std::runtime_error(tokens.at(0) + ": multiple port values");
	if (String::npos != tokens.at(1).find_first_not_of("0123456789"))
		throw std::runtime_error(tokens.at(0) + ": invalid port: not a number");
	size_t port = mzu::stringToInt(tokens.at(1));
	if (port < 1 || port > 65535)
		throw std::runtime_error(tokens.at(0) + ": invalid port: out of range");
	this->setPort(static_cast<int>(port));
	this->__portSet = true;
}

void Server::proccessHostToken(t_svec &tokens)
{
	if (this->__host != DEFAULT_HOST)
		throw std::runtime_error(tokens.at(0) + " directive is duplicate");
	if (tokens.size() == 1)
		throw std::runtime_error(tokens.at(0) + ": no host value");
	if (tokens.size() > 2)
		throw std::runtime_error(tokens.at(0) + ": multiple host values");
	this->__host = tokens.at(1);
}
