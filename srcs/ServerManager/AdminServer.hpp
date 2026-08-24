#ifndef __ADMINSERVER_HPP__
# define __ADMINSERVER_HPP__

# include "Server.hpp"

class AdminServer : public Server
{
	public:
		void							proccessToken( t_svec &tokens );

		AdminServer( String line );
		AdminServer( const AdminServer &copy );
		AdminServer	&operator=( const AdminServer &assign );
		~AdminServer();
};

#endif
