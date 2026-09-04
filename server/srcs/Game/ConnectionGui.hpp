#ifndef __CONNECTION_GUI_HPP__
# define __CONNECTION_GUI_HPP__

# include "Connection.hpp"

class ConnectionGui : public Connection
{
	private :
		void							processMessage(const String &message);

		ConnectionGui();
		ConnectionGui( const ConnectionGui &copy );
		ConnectionGui	&operator=( const ConnectionGui &assign );

	public:
		bool							readSocket();
		bool							writeSocket();
		String                          getCurrentGameState();

		ConnectionGui( Server *server );
		~ConnectionGui();
};

#endif
