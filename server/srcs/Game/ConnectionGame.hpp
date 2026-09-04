#ifndef __CONNECTION_GAME_HPP__
# define __CONNECTION_GAME_HPP__

# include "Connection.hpp"

class ConnectionGame : public Connection
{
	private :
		int								__playerId;
		e_player_state					__state;

		void							processMessage(const String &message);

		ConnectionGame();
		ConnectionGame( const ConnectionGame &copy );
		ConnectionGame	&operator=( const ConnectionGame &assign );

	public:
		e_player_state					getState() const;
		int								getPlayerId() const;
		void							setPlayerId(int id);
		void							setState(e_player_state state);

		bool							readSocket();
		bool							writeSocket();

		ConnectionGame( Server *server );
		~ConnectionGame();
};

#endif
