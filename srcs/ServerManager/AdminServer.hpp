#ifndef __ADMINSERVER_HPP__
# define __ADMINSERVER_HPP__

# include "Server.hpp"
# include "../zappy.hpp"
# include <openssl/ssl.h>
# include <openssl/err.h>

class AdminServer : public Server
{
	private:
		SSL_CTX 						*__ctx;
		String							__certificate;
		String							__privateKey;

		AdminServer( const AdminServer &copy );
		AdminServer	&operator=( const AdminServer &assign );

		void							proccessCertificateToken( t_svec &tokens );
		void							proccessPrivateKeyToken( t_svec &tokens );

	public:
		void							proccessToken( t_svec &tokens );
		SSL_CTX							*getTLSContext();
		void							setupSSL();

		AdminServer( String line );
		~AdminServer();
};

#endif
