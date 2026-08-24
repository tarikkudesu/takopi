#include "GuiServer.hpp"

GuiServer::GuiServer(String line)
{
	mzu::debug("GuiServer constructor");
	this->__type = "gui";
	parseBlock(line);
	if (!this->__portSet)
		throw std::runtime_error("gui server: missing \"port\" directive");
}
GuiServer::GuiServer(const GuiServer &copy) : Server(copy)
{
	mzu::debug("GuiServer copy constructor");
	*this = copy;
}
GuiServer &GuiServer::operator=(const GuiServer &assign)
{
	mzu::debug("GuiServer copy assignement operator");
	if (this != &assign)
	{
		Server::operator=(assign);
	}
	return *this;
}
GuiServer::~GuiServer()
{
	mzu::debug("GuiServer destructor");
}

void GuiServer::proccessToken(t_svec &tokens)
{
	String key = tokens.at(0);
	if (key != "port" && key != "host")
		throw std::runtime_error(key + ": unknown directive");
	if (key == "port")
		proccessPortToken(tokens);
	else
		proccessHostToken(tokens);
}
