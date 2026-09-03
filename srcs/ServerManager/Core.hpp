#ifndef __CORE_HPP__
# define __CORE_HPP__

#include "Server.hpp"
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
		static void					processConsoleInput();
		static void					writeDataToSocket( int sd );
		static void					readDataFromSocket( int sd );
		static void					acceptNewConnection( int sd );
		static int					buildSets(fd_set &readSet, fd_set &writeSet);
		static void					processConsoleCommand( const String &command );
		static void					proccessSelectEvent( int sd, fd_set &readSet, fd_set &writeSet, int &retV );
		static void					mainProcess();

		Core();
		Core( const Core &copy );
		Core	&operator=( const Core &assign );
		~Core();

	public:
		static void					logServers();
		static void					removeServer( int sd );
		static bool					isServerSocket( int sd );
		static void					removeConnection( int sd );
		static void					addServer( Server *server );
		static void					addConnection(Connection *connection);

		static bool					up;
		static void 				clear();
		static void					mainLoop();
};

#endif
