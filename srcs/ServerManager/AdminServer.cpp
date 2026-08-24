#include "AdminServer.hpp"

AdminServer::AdminServer(String line)
{
	mzu::debug("AdminServer constructor");
	this->__type = "admin";
	parseBlock(line);
	if (!this->__portSet)
		throw std::runtime_error("admin server: missing \"port\" directive");
}
AdminServer::AdminServer(const AdminServer &copy) : Server(copy)
{
	mzu::debug("AdminServer copy constructor");
	*this = copy;
}
AdminServer &AdminServer::operator=(const AdminServer &assign)
{
	mzu::debug("AdminServer copy assignement operator");
	if (this != &assign)
	{
		Server::operator=(assign);
	}
	return *this;
}
AdminServer::~AdminServer()
{
	mzu::debug("AdminServer destructor");
}

void AdminServer::proccessToken(t_svec &tokens)
{
	String key = tokens.at(0);
	if (key != "port" && key != "host")
		throw std::runtime_error(key + ": unknown directive");
	if (key == "port")
		proccessPortToken(tokens);
	else
		proccessHostToken(tokens);
}
