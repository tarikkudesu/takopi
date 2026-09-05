#include "CommandParser.hpp"

CommandParser::CommandParser() {}
CommandParser::CommandParser(const CommandParser &copy) { (void)copy; }
CommandParser &CommandParser::operator=(const CommandParser &assign) { (void)assign; return *this; }
CommandParser::~CommandParser() {}

t_command CommandParser::parseCommandType(const String &input)
{
	String cmd = input;
	size_t space = cmd.find(' ');
	if (space != String::npos)
		cmd = cmd.substr(0, space);

	if (cmd == "voir")			return CMD_VOIR;
	if (cmd == "avance")		return CMD_AVANCE;
	if (cmd == "droite")		return CMD_DROITE;
	if (cmd == "gauche")		return CMD_GAUCHE;
	if (cmd == "prend")			return CMD_PREND;
	if (cmd == "pose")			return CMD_POSE;
	if (cmd == "fork")			return CMD_FORK;
	if (cmd == "expulse")		return CMD_EXPULSE;
	if (cmd == "broadcast")		return CMD_BROADCAST;
	if (cmd == "inventaire")	return CMD_INVENTAIRE;
	if (cmd == "incantation")	return CMD_INCANTATION;
	if (cmd == "connect_nbr")	return CMD_CONNECT_NBR;
	if (cmd == "resize")		return CMD_ADMIN_RESIZE;
	if (cmd == "time")			return CMD_ADMIN_RETIME;
	if (cmd == "games")			return CMD_ADMIN_GAMES;
	if (cmd == "help")			return CMD_ADMIN_HELP;
	return CMD_UNKNOWN;
}

String CommandParser::parseArgument(const String &input)
{
	size_t space = input.find(' ');
	if (space == String::npos || space + 1 >= input.length())
		return "";
	return input.substr(space + 1);
}

String CommandParser::resourceName(e_resource res)
{
	switch (res)
	{
		case SIBUR:			return "sibur";
		case PHIRAS:		return "phiras";
		case LINEMATE:		return "linemate";
		case NOURRITURE:	return "nourriture";
		case DERAUMERE:		return "deraumere";
		case MENDIANE:		return "mendiane";
		case THYSTAME:		return "thystame";
		default:			return "";
	}
}

e_resource CommandParser::resourceFromName(const String &name)
{
	if (name == "sibur")		return SIBUR;
	if (name == "phiras")		return PHIRAS;
	if (name == "linemate")		return LINEMATE;
	if (name == "nourriture")	return NOURRITURE;
	if (name == "deraumere")	return DERAUMERE;
	if (name == "mendiane")		return MENDIANE;
	if (name == "thystame")		return THYSTAME;
	return RESOURCE_COUNT;
}
