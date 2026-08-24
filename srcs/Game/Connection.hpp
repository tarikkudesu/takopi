#ifndef __CONNECTION_HPP__
# define __CONNECTION_HPP__

# include "../ServerManager/Server.hpp"
# include "Client.hpp"

typedef std::map< int, Server* >			t_Server;
typedef std::vector< Server * >				t_serVect;

class Connection
{
	private :
		int								__sd;
		Client							__client;
		BasicString						__buffer;
		std::queue< BasicString >		__responseQueue;
		Server							*__server;

		Connection();
		Connection( const Connection &copy );
		Connection	&operator=( const Connection &assign );

	public:
		void							addData(const BasicString &input);
		void							setSocket( int sd );
		void							setServer(Server *server);
		int								getConnectionSocket();
		void							processData();
		bool							hasPendingOutput() const;
		const BasicString				&frontOutput() const;
		void							popOutput();

		Connection( Server *server );
		~Connection();
};

#endif
