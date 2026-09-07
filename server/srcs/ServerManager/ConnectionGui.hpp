#ifndef __CONNECTION_GUI_HPP__
# define __CONNECTION_GUI_HPP__

# include "Connection.hpp"

class Game;

class ConnectionGui : public Connection
{
	private :
		Game							*__game;
		t_gui_connection_state			__state;
		unsigned long					__eventCursor;

		void							processMessage(const String &message);

		ConnectionGui();
		ConnectionGui( const ConnectionGui &copy ) = delete;
		ConnectionGui	&operator=( const ConnectionGui &assign ) = delete;

	public:
		bool							readSocket();
		bool							writeSocket();
		bool							isReady() const;
		bool							isClosing() const;
		unsigned long					getEventCursor() const;
		String							getCurrentGameState() const;
		void							queueGameEvent(const s_gui_event &event);

		ConnectionGui( Server *server, Game *game );
		~ConnectionGui();
};

#endif
