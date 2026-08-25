#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include "Enums.hpp"

/*************************************************************************
 *                       CONNECTION CLIENT                               *
 *************************************************************************/

class Client
{
	private:
		e_player_state					__state;
		e_connection_type				__connectionType;
		int								__playerId;

	public:
		Client();
		Client(const Client &copy);
		Client							&operator=(const Client &assign);
		~Client();

		void							setConnectionType(e_connection_type type);
		void							setState(e_player_state state);
		e_connection_type				getConnectionType() const;
		int								getPlayerId() const;
		void							setPlayerId(int id);
		e_player_state					getState() const;
};

#endif