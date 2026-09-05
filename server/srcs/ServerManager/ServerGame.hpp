#ifndef __SERVERGAME_HPP__
# define __SERVERGAME_HPP__

# include "Server.hpp"

class Game;

class ServerGame : public Server
{
	private:
		Game								*__game;
		t_svec								__teams;
		int									__width;
		int									__height;
		int									__timeUnit;

		void								proccessTimeToken( t_svec &tokens );
		void								proccessWidthToken( t_svec &tokens );
		void								proccessTeamsToken( t_svec &tokens );
		void								proccessHeightToken( t_svec &tokens );
		ServerGame( const ServerGame &copy ) = delete;
		ServerGame	&operator=( const ServerGame &assign ) = delete;


	public:
		Game								*getGame();

		void								proccessToken( t_svec &tokens );
		void								initGame();

		ServerGame( String line );
		~ServerGame();
};

#endif
