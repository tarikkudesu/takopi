#ifndef __SERVERGAME_HPP__
# define __SERVERGAME_HPP__

# include "Server.hpp"

class Game;

class ServerGame : public Server
{
	private:
		t_svec								__teams;
		int									__width;
		int									__height;
		int									__timeUnit;

		void								proccessTimeToken( t_svec &tokens );
		void								proccessWidthToken( t_svec &tokens );
		void								proccessTeamsToken( t_svec &tokens );
		void								proccessHeightToken( t_svec &tokens );


	public:
		int									getMapWidth() const;
		int									getMapHeight() const;
		int									getTimeUnit() const;
		const t_svec						&getTeams() const;
		Game								*getGame();

		void								proccessToken( t_svec &tokens );
		void								initGame();

		ServerGame( String line );
		ServerGame( const ServerGame &copy );
		ServerGame	&operator=( const ServerGame &assign );
		~ServerGame();
};

#endif
