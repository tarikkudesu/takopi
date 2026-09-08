#ifndef __GAME_HPP__
#define __GAME_HPP__

#include "World.hpp"
#include "Command.hpp"
#include "Egg.hpp"
#include "Player.hpp"

/*************************************************************************
 *                         GAME ORCHESTRATOR                             *
 *************************************************************************/

class Game
{
	private:
		Game(const Game &copy) = delete;
		Game	&operator=(const Game &assign) = delete;

		t_game_state							__state;
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
		std::vector<s_gui_event>				__guiEvents;
		std::map<int, int>						__teamSlots;
		std::map<int, s_incantation_context>	__incantations;
		unsigned long							__nextGuiEventSequence;

		void									checkVictory();
		void									processFood(long currentTick);
		void									processEggs(long currentTick);
		void									processCommands(long currentTick);
		void									activateCommand(int playerId, const String &rawCmd, long currentTick);

		void									publishGuiEvent(const String &payload);
		Egg										*findOldestHatchedEgg(int teamIndex);
		int										countSameLevelPlayers(int playerId);
		int										availableSlots(int teamIndex) const;
		String									executeCommand(const Command &cmd);
		void									publishFullGuiState();

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
		t_game_state							getState() const;
		void									killPlayer(int playerId);

		Game();
		~Game();

		void									clearGuiEvents();
		void									clearNotifications();
		void									setTimeUnit(int timeUnit);
		void									resizeMap(int width, int height);
		void									enqueueCommand(int playerId, const String &rawCommand);
		String									executeAdminCommand(t_command type, const t_svec &args);
		String									handleHandshake(const String &teamName, int &outPlayerId);
		void									addNotification(int playerId, const String &msg);
		void									addGuiMessage(const String &message);
		void									removePlayer(int playerId);

		const World								&getWorld() const;
		const t_svec							&getTeams() const;
		const std::map<int, Player *>			&getPlayers() const;
		const std::vector<s_notification>		&getNotifications() const;
		const std::vector<s_gui_event>			&getGuiEvents() const;
		const std::vector<Egg *>				&getEggs() const;
		int										getTimeUnit() const;
		long									getCurrentTick() const;
		Player									*getPlayer(int playerId);
		const Player							*getPlayer(int playerId) const;
		unsigned long							getLastGuiEventSequence() const;
		const String							&getTeamName(int teamIndex) const;
		bool									canAcceptCommand(int playerId) const;
		int										getTeamIndex(const String &teamName) const;

		void									init(int width, int height, const t_svec &teams, int timeUnit);
		void									tick();
};

#endif
