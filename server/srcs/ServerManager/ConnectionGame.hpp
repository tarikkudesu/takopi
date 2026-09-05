#ifndef __CONNECTION_GAME_HPP__
# define __CONNECTION_GAME_HPP__

# include "Connection.hpp"

class ConnectionGame : public Connection
{
	private :
		e_player_state					__state;
		int								__playerId;

		void							processMessage(const String &message);

		ConnectionGame();
		ConnectionGame( const ConnectionGame &copy ) = delete;
		ConnectionGame	&operator=( const ConnectionGame &assign ) = delete;

	public:
		e_player_state					getState() const;
		int								getPlayerId() const;
		void							setState(e_player_state state);

		bool							readSocket();
		bool							writeSocket();

		ConnectionGame( Server *server );
		~ConnectionGame();
};

#endif
