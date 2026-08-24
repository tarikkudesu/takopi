#ifndef __GAMESERVER_HPP__
# define __GAMESERVER_HPP__

# include "Server.hpp"

class GameServer : public Server
{
	private:
		int								__width;
		int								__height;
		t_svec							__teams;

		void							proccessWidthToken( t_svec &tokens );
		void							proccessHeightToken( t_svec &tokens );
		void							proccessTeamsToken( t_svec &tokens );

	public:
		int								getMapWidth() const;
		int								getMapHeight() const;
		const t_svec					&getTeams() const;

		void							proccessToken( t_svec &tokens );

		GameServer( String line );
		GameServer( const GameServer &copy );
		GameServer	&operator=( const GameServer &assign );
		~GameServer();
};

#endif
