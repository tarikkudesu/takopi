#ifndef __CORE_HPP__
# define __CORE_HPP__

#include "../Game/Connection.hpp"

typedef std::map< int, Connection* >		t_Connections;


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
		static String				__consoleBuffer;

		static int					currentLoad();
		static void					mainProcess();
		static String				handleGamesCommand();
		static void					processConsoleInput();
		static void					writeDataToSocket( int sd );
		static void					readDataFromSocket( int sd );
		static void					acceptNewConnection( int sd );
		static void					processConsoleCommand( const String &command );
		static void					proccessSelectEvent( int sd, fd_set &readSet, fd_set &writeSet, int &retV );
		static String				routeGameCommand( t_command type, const t_svec &args );
		static int					buildSets( fd_set &readSet, fd_set &writeSet );

		Core();
		Core( const Core &copy );
		Core	&operator=( const Core &assign );
		~Core();

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
