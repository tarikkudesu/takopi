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
		String							__password;

		ServerAdmin( const ServerAdmin &copy );
		ServerAdmin	&operator=( const ServerAdmin &assign );

		void							proccessCertificateToken( t_svec &tokens );
		void							proccessPrivateKeyToken( t_svec &tokens );
		void							proccessPasswordToken( t_svec &tokens );

	public:
		void							proccessToken( t_svec &tokens );
		SSL_CTX							*getTLSContext();
		bool							checkPassword( const String &password ) const;
		void							setupSSL();

		ServerAdmin( String line );
		~ServerAdmin();
};

#endif
