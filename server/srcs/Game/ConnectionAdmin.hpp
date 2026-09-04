#ifndef __CONNECTION_ADMIN_HPP__
# define __CONNECTION_ADMIN_HPP__

# include "Connection.hpp"
# include <openssl/ssl.h>

class ConnectionAdmin : public Connection
{
	private :
		size_t							__responseOffset;
		bool							__tlsHandshake;
		e_tls_operation					__tlsOperation;
		bool							__tlsFailed;
		e_tls_wait						__tlsWait;
		SSL								*__ssl;

		void							processMessage(const String &message);

		ConnectionAdmin();
		ConnectionAdmin( const ConnectionAdmin &copy );
		ConnectionAdmin	&operator=( const ConnectionAdmin &assign );

	public:
		bool							usesTLS() const;
		void							setupTLS(SSL_CTX *ctx);
		bool							processTLSHandshake();
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
