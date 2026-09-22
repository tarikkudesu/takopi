#ifndef __CONNECTION_ADMIN_HPP__
# define __CONNECTION_ADMIN_HPP__

# include "Connection.hpp"
# include <openssl/ssl.h>

class ConnectionAdmin : public Connection
{
	private :
		SSL								*__ssl;
		t_admin_connection_state		__state;
		t_tls_wait						__tlsWait;
		t_tls_operation					__tlsOperation;
		unsigned int					__authenticationFailures;
		size_t							__responseOffset;

		void							processMessage(const String &message);

		ConnectionAdmin();
		ConnectionAdmin( const ConnectionAdmin &copy ) = delete;
		ConnectionAdmin	&operator=( const ConnectionAdmin &assign ) = delete;

	public:
		bool							usesTLS() const;
		bool							tlsNeedsRead() const;
		bool							tlsNeedsWrite() const;
		bool							processTLSHandshake();
		void							setupTLS(SSL_CTX *ctx);
		bool							tlsHandshakeFailed() const;
		bool							tlsHandshakePending() const;
		t_admin_connection_state		getState() const;

		bool							readSocket();
		bool							writeSocket();

		void							popOutput();
		size_t							responseOffset() const;
		void							setResponseOffset(size_t offset);

		ConnectionAdmin( Server *server );
		~ConnectionAdmin();
};

#endif
