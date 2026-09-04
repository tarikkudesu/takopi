#ifndef __CONNECTION_HPP__
#define __CONNECTION_HPP__

#include "../zappy.hpp"
#include "../utilities/BasicString.hpp"

class Connection
{
	private:
		int								__sd;
		BasicString						__inBuffer;
		std::queue<String>				__inbox;
		std::queue<BasicString>			__outQueue;
		size_t							__outOffset;

		void							extractLines();

	public:
		Connection();
		Connection( const Connection &copy );
		Connection	&operator=( const Connection &assign );
		~Connection();

		void							connectTo( const String &host, int port );
		int								getSocket() const;

		bool							readSocket();
		bool							writeSocket();

		bool							hasInboundLine() const;
		String							popInboundLine();

		bool							hasPendingOutput() const;
		void							pushOutput( const String &line );
};

#endif
