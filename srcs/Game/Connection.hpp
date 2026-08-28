#ifndef __CONNECTION_HPP__
# define __CONNECTION_HPP__

# include "../ServerManager/Server.hpp"
# include "Enums.hpp"
# include <openssl/ssl.h>

class Connection
{
	private :
		int								__sd;
		Server							*__server;
		e_connection_type				__connectionType;
		int								__playerId;
		e_player_state					__state;

		BasicString						__buffer;
		std::queue< BasicString >		__responseQueue;
		size_t							__responseOffset;

		SSL								*__ssl;
		enum e_tls_operation
		{
			TLS_OPERATION_NONE,
			TLS_OPERATION_READ,
			TLS_OPERATION_WRITE,
			TLS_OPERATION_HANDSHAKE
		};
		enum e_tls_wait
		{
			TLS_WAIT_READ,
			TLS_WAIT_WRITE
		};
		bool							__tlsHandshake;
		e_tls_operation					__tlsOperation;
		e_tls_wait						__tlsWait;
		bool							__tlsFailed;

		void							processMessage(const String &message);
		void							processGameMessage(const String &message);

		Connection();
		Connection( const Connection &copy );
		Connection	&operator=( const Connection &assign );

	public:
		void							addData(const BasicString &input);
		void							processData();
		void							setSocket( int sd );
		int								getConnectionSocket();
		e_connection_type				getConnectionType() const;
		e_player_state					getState() const;
		void							setState(e_player_state state);
		int								getPlayerId() const;
		void							setPlayerId(int id);


		bool							usesTLS() const;
		void							setupTLS(SSL_CTX *ctx);
		bool							processTLSHandshake();
		bool							tlsNeedsRead() const;
		bool							tlsNeedsWrite() const;
		bool							tlsReadPending() const;
		bool							tlsWritePending() const;
		bool							tlsHandshakeFailed() const;
		bool							tlsHandshakePending() const;
		ssize_t							readSocket(char *buffer, size_t size);
		ssize_t							writeSocket(const char *buffer, size_t size);

		void							popOutput();
		const BasicString				&frontOutput() const;
		size_t							responseOffset() const;
		bool							hasPendingOutput() const;
		void							pushOutput(const String &msg);
		void							setResponseOffset(size_t offset);

		Connection( Server *server );
		~Connection();
};

#endif
