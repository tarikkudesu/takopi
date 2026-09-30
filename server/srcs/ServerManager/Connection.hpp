#ifndef __CONNECTION_HPP__
# define __CONNECTION_HPP__

# include "../utilities/BasicString.hpp"

class Server;

class Connection
{
	protected:
		int								__sd;
		Type							__type;
		BasicString						__buffer;
		Server							*__server;
		std::queue< BasicString >		__responseQueue;

		virtual void					processMessage(const String &message) = 0;

		Connection( Server *server );
		Connection( const Connection &copy ) = delete;
		Connection	&operator=( const Connection &assign ) = delete;
		

	public:
		void							processData();
		Type							getType() const;
		Server							*getServer() const;
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
