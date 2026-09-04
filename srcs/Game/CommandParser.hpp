#ifndef __COMMANDPARSER_HPP__
#define __COMMANDPARSER_HPP__

#include "Command.hpp"

class CommandParser
{
	private:
		CommandParser();
		CommandParser(const CommandParser &copy);
		CommandParser					&operator=(const CommandParser &assign);
		~CommandParser();

	public:
		static e_command				parseCommandType(const String &input);
		static String					parseArgument(const String &input);
		static String					resourceName(e_resource res);
		static e_resource				resourceFromName(const String &name);
};

#endif
