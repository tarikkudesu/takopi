#ifndef __COMMANDPARSER_HPP__
#define __COMMANDPARSER_HPP__

#include "../zappy.hpp"

class CommandParser
{
	private:
		CommandParser() = delete;
		CommandParser(const CommandParser &copy) = delete;
		CommandParser	&operator=(const CommandParser &assign) = delete;
		~CommandParser() = delete;

	public:
		static CommandType				parseCommandType(const String &input);
		static Resource					resourceFromName(const String &name);
		static String					parseArgument(const String &input);
};

#endif
