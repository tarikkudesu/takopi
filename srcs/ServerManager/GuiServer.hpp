#ifndef __GUISERVER_HPP__
# define __GUISERVER_HPP__

# include "Server.hpp"

class GuiServer : public Server
{
	public:
		void							proccessToken( t_svec &tokens );

		GuiServer( String line );
		GuiServer( const GuiServer &copy );
		GuiServer	&operator=( const GuiServer &assign );
		~GuiServer();
};

#endif
