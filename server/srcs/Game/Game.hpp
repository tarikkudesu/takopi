#ifndef __GAME_HPP__
#define __GAME_HPP__

#include "World.hpp"

/*************************************************************************
 *                         GAME ORCHESTRATOR                             *
 *************************************************************************/

class Game
{
	private:
		t_svec									__teams;
		World									__world;
		int										__timeUnit;
		int										__nextEggId;
		int										__nextPlayerId;
		double									__tickOffset;
		struct timeval							__startTime;

		std::vector<Egg *>						__eggs;
		std::map<int, Player *>					__players;
		std::vector<s_notification>				__notifications;
		std::map<int, std::queue<String> >		__pendingCommands;
		std::map<int, Command>					__activeCommands;
		std::map<int, int>						__teamSlots;

		void									killPlayer(int playerId);
		void									processFood(long currentTick);
		void									processEggs(long currentTick);
		void									processCommands(long currentTick);
		void									activateCommand(int playerId, const String &rawCmd, long currentTick);

		String									executeCommand(const Command &cmd);
		int										countSameLevelPlayers(int playerId);

		String									executeVoir(int playerId);
		String									executeFork(int playerId);
		String									executeAvance(int playerId);
		String									executeDroite(int playerId);
		String									executeGauche(int playerId);
		String									executeExpulse(int playerId);
		String									executeConnectNbr(int playerId);
		String									executeInventaire(int playerId);
		String									executeIncantation(int playerId);
		String									executePoser(int playerId, const String &object);
		String									executeBroadcast(int playerId, const String &text);
		String									executePrendre(int playerId, const String &object);

	public:
		Game();
		Game(const Game &copy);
		Game	&operator=(const Game &assign);
		~Game();

		void									clearNotifications();
		void									setTimeUnit(int timeUnit);
		void									resizeMap(int width, int height);
		void									enqueueCommand(int playerId, const String &rawCommand);
		String									executeAdminCommand(t_command type, const t_svec &args);
		String									handleHandshake(const String &teamName, int &outPlayerId);
		void									addNotification(int playerId, const String &msg);

		const std::vector<s_notification>		&getNotifications() const;
		Player									*getPlayer(int playerId);
		long									getCurrentTick() const;
		int										getMapHeight() const;
		int										getMapWidth() const;
		int										getTimeUnit() const;
		int										getTeamIndex(const String &teamName) const;
		int										getRemainingSlots(int teamIndex) const;
		bool									canAcceptCommand(int playerId) const;

		void									init(int width, int height, const t_svec &teams, int timeUnit);
		void									tick();
};

#endif
