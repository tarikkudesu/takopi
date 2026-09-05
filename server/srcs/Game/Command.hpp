#ifndef __COMMAND_HPP__
#define __COMMAND_HPP__

#include "Egg.hpp"

/*************************************************************************
 *                         GAME COMMAND                                  *
 *************************************************************************/

class Command
{
	private:
		t_command						__type;
		String							__argument;
		long							__executionTime;
		int								__playerId;

	public:
		Command();
		Command(t_command type, const String &argument, long executionTime, int playerId);
		Command(const Command &copy);
		Command							&operator=(const Command &assign);
		~Command();

		t_command						getType() const;
		int								getPlayerId() const;
		const String					&getArgument() const;
		long							getExecutionTime() const;
		bool							isReady(long currentTick) const;

		static int						durationForCommand(t_command type);
};

#endif
