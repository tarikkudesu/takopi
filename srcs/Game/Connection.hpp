#ifndef __CONNECTION_HPP__
# define __CONNECTION_HPP__

# include "../ServerManager/Server.hpp"
# include "Enums.hpp"
# include <openssl/ssl.h>

class Connection
{
	protected:
		int								__sd;
		e_type							__type;
		BasicString						__buffer;
		Server							*__server;
		std::queue< BasicString >		__responseQueue;

		virtual void					processMessage(const String &message) = 0;

		Connection( Server *server );
		Connection( const Connection &copy );
		Connection	&operator=( const Connection &assign );
		

	public:
		void							processData();
		e_type							getType() const;
		void							setSocket( int sd );
		int								getConnectionSocket();
		void							addData(const BasicString &input);

		virtual bool					readSocket() = 0;
		virtual bool					writeSocket() = 0;

		virtual void					popOutput();
		const BasicString				&frontOutput() const;
		bool							hasPendingOutput() const;
		void							pushOutput(const String &msg);

		virtual ~Connection();
};

#endif
