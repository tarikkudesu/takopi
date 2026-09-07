#ifndef __CONNECTION_GUI_HPP__
# define __CONNECTION_GUI_HPP__

# include "Connection.hpp"

class Game;

class ConnectionGui : public Connection
{
	private :
		Game							*__game;

		void							processMessage(const String &message);

		ConnectionGui();
		ConnectionGui( const ConnectionGui &copy ) = delete;
		ConnectionGui	&operator=( const ConnectionGui &assign ) = delete;

	public:
		bool							readSocket();
		bool							writeSocket();
		String                          getCurrentGameState();

		ConnectionGui( Server *server, Game *game );
		~ConnectionGui();
};

#endif
