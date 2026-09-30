#ifndef __CONNECTION_GAME_HPP__
# define __CONNECTION_GAME_HPP__

# include "Connection.hpp"

class ConnectionGame : public Connection
{
	private :
		PlayerState						__state;
		int								__playerId;

		void							processMessage(const String &message);

		ConnectionGame();
		ConnectionGame( const ConnectionGame &copy ) = delete;
		ConnectionGame	&operator=( const ConnectionGame &assign ) = delete;

	public:
		PlayerState						getState() const;
		int								getPlayerId() const;
		void							setState(PlayerState state);

		bool							readSocket();
		bool							writeSocket();

		ConnectionGame( Server *server );
		~ConnectionGame();
};

#endif
