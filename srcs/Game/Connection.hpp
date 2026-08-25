#ifndef __CONNECTION_HPP__
# define __CONNECTION_HPP__

# include "../ServerManager/Server.hpp"
# include "Client.hpp"

class Connection
{
	private :
		int								__sd;
		Server							*__server;
		Client							__client;
		BasicString						__buffer;
		std::queue< BasicString >		__responseQueue;

		Connection();
		Connection( const Connection &copy );
		Connection	&operator=( const Connection &assign );

	public:
		void							addData(const BasicString &input);
		int								getConnectionSocket();
		void							setSocket( int sd );
		void							processData();

		bool							hasPendingOutput() const;
		const BasicString				&frontOutput() const;
		void							popOutput();
		void							pushOutput(const String &msg);

		Client							&getClient();
		const Client					&getClient() const;

		Connection( Server *server );
		~Connection();
};

#endif
