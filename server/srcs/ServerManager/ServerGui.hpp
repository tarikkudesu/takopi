#ifndef __SERVERGUI_HPP__
# define __SERVERGUI_HPP__

# include "Server.hpp"

class ServerGui : public Server
{
	public:
		void							proccessToken( t_svec &tokens );

		ServerGui( String line );
		ServerGui( const ServerGui &copy );
		ServerGui	&operator=( const ServerGui &assign );
		~ServerGui();
};

#endif
