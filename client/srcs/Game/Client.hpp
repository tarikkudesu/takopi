#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include "../Network/Connection.hpp"
#include "Strategy.hpp"

/*************************************************************************
 *                            HANDSHAKE STATE                            *
 *************************************************************************/

typedef enum e_handshake_state
{
	HS_WAIT_WELCOME = 0,
	HS_WAIT_SLOTS,
	HS_WAIT_SIZE,
	HS_PLAYING,
	HS_DEAD
} t_handshake_state;

typedef struct s_pending_command
{
	e_command						type;
	String							argument;
} t_pending_command;

/*************************************************************************
 *                          CLIENT ORCHESTRATOR                          *
 *************************************************************************/

class Client
{
	private:
		String							__team;
		String							__host;
		int								__port;
		int								__worldWidth;
		int								__worldHeight;
		Connection						__connection;
		Strategy						__strategy;
		t_handshake_state				__state;
		bool							__hasPending;
		t_pending_command				__pending;
		bool							__incantationPending;

		Client();

		void							onWelcome( const String &line );
		void							onSlots( const String &line );
		void							onSize( const String &line );
		void							handlePlayingLine( const String &line );
		void							handleGenericReply( const String &line );
		void							sendCommand( const String &commandLine );
		void							sendNextCommand();

	public:
		Client( const String &team, const String &host, int port );
		Client( const Client &copy );
		Client							&operator=( const Client &assign );
		~Client();

		void							run();

		static bool						up;
		static void						stop();
};

#endif
