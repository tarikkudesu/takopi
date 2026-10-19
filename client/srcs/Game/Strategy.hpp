#ifndef __STRATEGY_HPP__
#define __STRATEGY_HPP__

#include "Protocol.hpp"
#include "Elevation.hpp"
#include "BroadcastMessenger.hpp"
#include "SoundNavigator.hpp"
#include "ElevationManager.hpp"
#include "IState.hpp"
#include <vector>
#include <set>
#include <map>

class Strategy
{
	private:
		int								__inventory[RESOURCE_COUNT];
		int								__level;
		bool							__hasInventory;
		bool							__hasVision;
		int								__ticksSinceInventory;
		std::vector<t_tile_content>		__vision;

		String							__teamName;
		int								__myId;

		// Egg / fork management
		bool							__forked;
		std::set<int>					__knownTeammates;
		int								__eggsLaidCount;

		// Confirmed teammates on tile for incantation (id -> ticks since last HERE)
		std::map<int, int>				__confirmedFollowersTick;

		// Coordination fields
		int								__targetLeaderId;
		int								__targetLeaderDirection;
		int								__targetLeaderLevel;
		int								__leaderHeartbeat;
		int								__broadcastTimer;
		int								__gatherTimeout;
		int								__gatherWaitTicks;
		int								__gatherCooldown;
		int								__consecutiveTurns;
		int								__consecutiveAdvances;
		int								__totalTeamEggs;
		bool							__eggLaidAtThisLevel;
		int								__activeLeaderForLevel[WIN_LEVEL + 1];
		int								__lastOfferTick[WIN_LEVEL + 1];
		std::vector<String>				__queuedCommands;

		// Current State pointer (IState interface)
		IState*							__currentState;

		// Static state instances
		IState*							__stateSolo;
		IState*							__stateSurvive;
		IState*							__stateForage;
		IState*							__stateLeaderGather;
		IState*							__stateJoinLeader;
		IState*							__stateIncanting;

		BroadcastMessenger				__messenger;

		void							initStates();
		void							cleanupStates();

	public:
		Strategy();
		Strategy( const Strategy &copy );
		Strategy						&operator=( const Strategy &assign );
		~Strategy();

		void							setTeamName( const String &team );
		String							decideNextCommand();

		void							applyVision( const String &rawLine );
		void							applyInventory( const String &rawLine );
		void							applyLevel( int level );
		void							applyPrendResult( e_resource resource, bool ok );
		void							applyPoseResult( e_resource resource, bool ok );
		void							invalidateVision();
		void							applyBroadcast( int direction, const String &text );

		// State management
		void							transitionTo( IState *newState );
		IState*							getStateSolo() const;
		IState*							getStateSurvive() const;
		IState*							getStateForage() const;
		IState*							getStateLeaderGather() const;
		IState*							getStateJoinLeader() const;
		IState*							getStateIncanting() const;
		IState*							getCurrentState() const;

		// Getters & mutators for states
		int								getLevel() const;
		int								getMyId() const;
		const int*						getInventory() const;
		const std::vector<t_tile_content>& getVision() const;
		bool							hasVision() const;
		bool							isForked() const;
		void							setForked( bool f );
		int								getTeamSizeEstimate() const;
		int								getTotalTeamEggs() const;
		bool							hasEggLaidAtThisLevel() const;
		bool							canLayEgg() const;
		void							recordEggLaid();
		int								getEggsLaidCount() const;
		void							addKnownTeammate( int id );
		bool							hasActiveLeaderForLevel( int level ) const;
		int								getActiveLeaderForLevel( int level ) const;
		void							setActiveLeaderForLevel( int level, int leaderId );
		int								getConfirmedFollowersCount() const;
		void							addConfirmedFollower( int id );
		void							clearConfirmedFollowers();
		int								getTargetLeaderId() const;
		void							setTargetLeaderId( int id );
		int								getTargetLeaderDirection() const;
		void							setTargetLeaderDirection( int dir );
		int								getLeaderHeartbeat() const;
		void							resetLeaderHeartbeat();
		int								incrementLeaderHeartbeat();
		int								incrementBroadcastTimer();
		void							resetBroadcastTimer();
		int								getGatherTimeout() const;
		void							resetGatherTimeout();
		int								incrementGatherTimeout();
		int								getGatherWaitTicks() const;
		void							resetGatherWaitTicks();
		int								incrementGatherWaitTicks();
		int								getConsecutiveTurns() const;
		void							incrementConsecutiveTurns();
		void							resetConsecutiveTurns();
		int								getGatherCooldown() const;
		void							setGatherCooldown( int cd );
		String							getQueuedCommand() const;
		void							setQueuedCommand( const String &cmd );
		void							queueCommand( const String &cmd );
		BroadcastMessenger&				getMessenger();
};

#endif
