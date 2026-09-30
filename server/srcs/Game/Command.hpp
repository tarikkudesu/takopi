#ifndef __COMMAND_HPP__
#define __COMMAND_HPP__

#include "../zappy.hpp"

class Command
{
	private:
		CommandType						__type;
		String							__argument;
		long							__executionTime;
		int								__playerId;

	public:
		Command();
		Command(CommandType type, const String &argument, long executionTime, int playerId);
		Command(const Command &copy);
		Command	&operator=(const Command &assign);
		~Command();

		CommandType						getType() const;
		int								getPlayerId() const;
		const String					&getArgument() const;
		long							getExecutionTime() const;

		static int						durationForCommand(CommandType type);
};

#endif
