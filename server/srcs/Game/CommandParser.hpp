#ifndef __COMMANDPARSER_HPP__
#define __COMMANDPARSER_HPP__

#include "../zappy.hpp"

class CommandParser
{
	private:
		CommandParser() = delete;
		CommandParser(const CommandParser &copy) = delete;
		CommandParser					&operator=(const CommandParser &assign) = delete;
		~CommandParser() = delete;

	public:
		static t_command				parseCommandType(const String &input);
		static String					parseArgument(const String &input);
		static e_resource				resourceFromName(const String &name);
};

#endif
