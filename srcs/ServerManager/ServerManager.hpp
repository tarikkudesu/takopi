#ifndef __SERVERMANAGER_HPP__
# define __SERVERMANAGER_HPP__

# include "../Game/Game.hpp"
# include "Core.hpp"

/*************************************************************************
 *                             SERVER LAUNCHER                           *
 *************************************************************************/

class ServerManager
{
	private :
		GameConfig						__config;

		void							applyFileFallback();
		void							launch();
		void							parseConfigFile(const String &path);

		ServerManager();
		ServerManager( const ServerManager &copy );
		ServerManager	&operator=( const ServerManager &assign );

	public:
		void							setUpZappy();

		ServerManager( const GameConfig &config );
		~ServerManager();
};

#endif
