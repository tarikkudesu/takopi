#ifndef __GAME_HPP__
#define __GAME_HPP__

#include "World.hpp"

/*************************************************************************
 *                         GAME ORCHESTRATOR                             *
 *************************************************************************/

class Game
{
	private:
		World								__world;
		t_svec								__teams;
		std::vector<Egg *>					__eggs;
		std::map<int, Player *>				__players;
		int									__timeUnit;
		std::map<int, int>					__teamSlots;
		struct timeval						__startTime;
		int									__clientsPerTeam;
		int									__nextPlayerId;
		int									__nextEggId;
		std::map<int, Command>				__activeCommands;
		std::map<int, std::queue<String> >	__pendingCommands;
		std::vector<s_notification>			__notifications;

		void				processFood(long currentTick);
		void				processEggs(long currentTick);
		void				processCommands(long currentTick);
		void				activateCommand(int playerId, const String &rawCmd, long currentTick);
		String				executeCommand(const Command &cmd);
		void				killPlayer(int playerId);
		int					countSameLevelPlayers(int playerId);

		String				executeAvance(int playerId);
		String				executeDroite(int playerId);
		String				executeGauche(int playerId);
		String				executeVoir(int playerId);
		String				executeInventaire(int playerId);
		String				executePrendre(int playerId, const String &object);
		String				executePoser(int playerId, const String &object);
		String				executeExpulse(int playerId);
		String				executeBroadcast(int playerId, const String &text);
		String				executeIncantation(int playerId);
		String				executeFork(int playerId);
		String				executeConnectNbr(int playerId);

	public:
		Game();
		Game(const Game &copy);
		Game				&operator=(const Game &assign);
		~Game();

		void									init(int width, int height, const t_svec &teams, int timeUnit, int clientsPerTeam);
		long									getCurrentTick() const;
		void									tick();

		const std::vector<s_notification>		&getNotifications() const;
		String									handleHandshake(const String &teamName, int &outPlayerId);
		void									enqueueCommand(int playerId, const String &rawCommand);
		void									addNotification(int playerId, const String &msg);
		int										getTeamIndex(const String &teamName) const;
		int										getRemainingSlots(int teamIndex) const;
		bool									canAcceptCommand(int playerId) const;
		Player									*getPlayer(int playerId);
		void									clearNotifications();
};

#endif
