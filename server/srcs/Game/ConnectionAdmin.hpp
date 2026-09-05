#ifndef __CONNECTION_ADMIN_HPP__
# define __CONNECTION_ADMIN_HPP__

# include "Connection.hpp"
# include <openssl/ssl.h>

class ConnectionAdmin : public Connection
{
	private :
		SSL								*__ssl;
		t_tls_wait						__tlsWait;
		t_tls_operation					__tlsOperation;
		unsigned int					__authenticationFailures;
		size_t							__responseOffset;
		bool							__authenticated;
		bool							__tlsHandshake;
		bool							__tlsFailed;
		bool							__closing;

		void							processMessage(const String &message);

		ConnectionAdmin();
		ConnectionAdmin( const ConnectionAdmin &copy );
		ConnectionAdmin	&operator=( const ConnectionAdmin &assign );

	public:
		bool							usesTLS() const;
		bool							processTLSHandshake();
		void							setupTLS(SSL_CTX *ctx);
		bool							tlsNeedsRead() const;
		bool							tlsNeedsWrite() const;
		bool							tlsReadPending() const;
		bool							tlsWritePending() const;
		bool							tlsHandshakeFailed() const;
		bool							tlsHandshakePending() const;

		bool							readSocket();
		bool							writeSocket();

		void							popOutput();
		size_t							responseOffset() const;
		void							setResponseOffset(size_t offset);

		ConnectionAdmin( Server *server );
		~ConnectionAdmin();
};

#endif
