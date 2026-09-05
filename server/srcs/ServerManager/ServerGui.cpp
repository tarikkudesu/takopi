#include "ServerGui.hpp"

ServerGui::ServerGui(String line) : Server(GUI)
{
	mzu::debug("ServerGui constructor");
	parseBlock(line);
	if (!this->__portSet)
		throw std::runtime_error("gui server: missing \"port\" directive");
}
ServerGui::~ServerGui()
{
	mzu::debug("ServerGui destructor");
}

void ServerGui::proccessToken(t_svec &tokens)
{
	String key = tokens.at(0);
	if (key != "port" && key != "host")
		throw std::runtime_error(key + ": unknown directive");
	if (key == "port")
		proccessPortToken(tokens);
	else
		proccessHostToken(tokens);
}
