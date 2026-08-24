#ifndef __SERVER_HPP__
# define __SERVER_HPP__

# include "../utilities/BasicString.hpp"

class Server
{
	private:
		int								__sd;
		int								__port;
		String							__host;

		Server();
		Server( const Server &copy );
		Server	&operator=( const Server &assign );

	public:
		void							setup();

		int								getServerSocket() const;
		String							serverIdentity() const;
		const String					&getServerHost() const;
		int								getServerPort() const;
		void							setPort(int port);

		Server(int port, const String &host);
		~Server();
};

#endif
