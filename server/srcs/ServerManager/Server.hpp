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
		e_type							__type;
		bool							__portSet;
		t_svec							__directives;

		void							parseBlock( String line );
		void							proccessServerDirectives();
		void							parseServerDirectives( String line );
		void							addServerDirective( String &line, size_t end );
		void							proccessPortToken( t_svec &tokens );
		void							proccessHostToken( t_svec &tokens );
		virtual void					proccessToken( t_svec &tokens ) = 0;

		Server(e_type type);
		Server( const Server &copy ) = delete;
		Server	&operator=( const Server &assign ) = delete;

	public:
		void							setup();

		int								getServerSocket() const;
		const String					&getServerHost() const;
		String							serverIdentity() const;
		int								getServerPort() const;
		void							setPort(int port);
		const e_type					&getType() const;

		virtual ~Server();
};

#endif
