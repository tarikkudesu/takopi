#ifndef __SERVER_HPP__
# define __SERVER_HPP__

# include "../utilities/BasicString.hpp"

class Server;

typedef std::map< int, Server* >			t_Server;
typedef std::vector< Server * >				t_serVect;

class Server
{
	private:
		int								__sd;
		int								__port;
		String							__host;

	protected:
		String							__type;
		bool							__portSet;
		t_svec							__directives;

		void							parseBlock( String line );
		void							parseServerDirectives( String line );
		void							addServerDirective( String &line, size_t end );
		void							proccessServerDirectives();
		void							proccessPortToken( t_svec &tokens );
		void							proccessHostToken( t_svec &tokens );
		virtual void					proccessToken( t_svec &tokens ) = 0;

		Server();
		Server( const Server &copy );
		Server	&operator=( const Server &assign );

	public:
		void							setup();

		int								getServerSocket() const;
		int								getServerPort() const;
		const String					&getServerHost() const;
		const String					&getServerType() const;
		String							serverIdentity() const;
		void							setPort(int port);

		virtual ~Server();
};

#endif
