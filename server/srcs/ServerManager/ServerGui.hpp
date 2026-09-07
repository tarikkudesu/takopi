#ifndef __SERVERGUI_HPP__
# define __SERVERGUI_HPP__

# include "Server.hpp"

class ServerGui : public Server
{
	private:
		ServerGui( const ServerGui &copy ) = delete;
		ServerGui	&operator=( const ServerGui &assign ) = delete;

	public:
		void							proccessToken( t_svec &tokens );
		void							furtherSetup();

		ServerGui( String line );
		~ServerGui();
};

#endif
