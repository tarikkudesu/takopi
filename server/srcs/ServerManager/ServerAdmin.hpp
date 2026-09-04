#ifndef __SERVERADMIN_HPP__
# define __SERVERADMIN_HPP__

# include "Server.hpp"
# include "../zappy.hpp"
# include <openssl/ssl.h>
# include <openssl/err.h>

class ServerAdmin : public Server
{
	private:
		SSL_CTX 						*__ctx;
		String							__certificate;
		String							__privateKey;

		ServerAdmin( const ServerAdmin &copy );
		ServerAdmin	&operator=( const ServerAdmin &assign );

		void							proccessCertificateToken( t_svec &tokens );
		void							proccessPrivateKeyToken( t_svec &tokens );

	public:
		void							proccessToken( t_svec &tokens );
		SSL_CTX							*getTLSContext();
		void							setupSSL();

		ServerAdmin( String line );
		~ServerAdmin();
};

#endif
