#ifndef __GAMESERVER_HPP__
# define __GAMESERVER_HPP__

# include "Server.hpp"

class Game;

class GameServer : public Server
{
	private:
		int									__width;
		int									__height;
		int									__timeUnit;
		int									__clientsPerTeam;
		t_svec								__teams;
		Game								*__game;

		void								proccessWidthToken( t_svec &tokens );
		void								proccessHeightToken( t_svec &tokens );
		void								proccessTeamsToken( t_svec &tokens );
		void								proccessTimeToken( t_svec &tokens );
		void								proccessClientsToken( t_svec &tokens );

	public:
		int									getMapWidth() const;
		int									getMapHeight() const;
		int									getTimeUnit() const;
		int									getClientsPerTeam() const;
		const t_svec						&getTeams() const;
		Game								*getGame();

		void								proccessToken( t_svec &tokens );
		void								initGame();

		GameServer( String line );
		GameServer( const GameServer &copy );
		GameServer	&operator=( const GameServer &assign );
		~GameServer();
};

#endif
