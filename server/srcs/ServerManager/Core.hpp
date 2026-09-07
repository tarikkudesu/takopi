#ifndef __CORE_HPP__
# define __CORE_HPP__

#include "Connection.hpp"
#include "Server.hpp"

typedef std::map< int, Connection* >		t_Connections;

class Game;

/************************************************************************
 *                            SERVER CONTROL                            *
 ************************************************************************/

class Core
{
	private:
		static t_Server				__servers;
		static t_Connections		__connections;
		static bool					__consoleOpen;
		static bool					__consoleDiscarding;
		static bool					__criticalOverload;
		static String				__consoleBuffer;

		static Game					*getGame();
		static int					currentLoad();
		static void					mainProcess();
		static void					processConsoleInput();
		static void					writeDataToSocket( int sd );
		static void					readDataFromSocket( int sd );
		static void					acceptNewConnection( int sd );
		static void					processConsoleCommand( const String &command );
		static void					proccessSelectEvent( int sd, fd_set &readSet, fd_set &writeSet, int &retV );
		static String				executeGameCommand( t_command type, const t_svec &args );
		static int					buildSets( fd_set &readSet, fd_set &writeSet );

		Core() = delete;
		Core( const Core &copy ) = delete;
		Core	&operator=( const Core &assign ) = delete;
		~Core() = delete;

	public:
		static void					logServers();
		static bool					hasGameServer();
		static void					removeServer( int sd );
		static bool					isServerSocket( int sd );
		static void					removeConnection( int sd );
		static void					addServer( Server *server );
		static void					addConnection(Connection *connection);
		static String				executeAdminCommand( const String &command );

		static bool					up;
		static void 				clear();
		static void					mainLoop();
};

#endif
