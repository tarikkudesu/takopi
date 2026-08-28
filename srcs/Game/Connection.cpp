#include "Connection.hpp"
#include "../ServerManager/GameServer.hpp"
#include "../Game/Game.hpp"

Connection::Connection(Server *server) : __sd(-1), __server(server), __connectionType(CONNECTION_UNKNOWN), __playerId(-1), __state(PLAYER_HANDSHAKE), __responseOffset(0), __ssl(NULL), __tlsHandshake(false), __tlsOperation(TLS_OPERATION_NONE), __tlsWait(TLS_WAIT_READ), __tlsFailed(false)
{
	mzu::debug("Connection constructor");
}
Connection::Connection(const Connection &copy) : __sd(-1), __server(NULL)
{
	mzu::debug("Connection copy constructor");
	*this = copy;
}
Connection &Connection::operator=(const Connection &assign)
{
	mzu::debug("Connection copy assignement operator");
	if (this != &assign)
	{
		__ssl = NULL;
		__sd = assign.__sd;
		__tlsFailed = false;
		__tlsHandshake = false;
		__tlsOperation = TLS_OPERATION_NONE;
		__tlsWait = TLS_WAIT_READ;
		__state = assign.__state;
		__server = assign.__server;
		__playerId = assign.__playerId;
		__responseQueue = assign.__responseQueue;
		__responseOffset = assign.__responseOffset;
		__connectionType = assign.__connectionType;
	}
	return *this;
}
Connection::~Connection()
{
	if (__ssl)
	{
		SSL_shutdown(__ssl);
		SSL_free(__ssl);
	}
	mzu::debug("Connection destructor");
}

void Connection::setupTLS(SSL_CTX *ctx)
{
	if (!ctx || __sd < 0)
		throw std::runtime_error("admin connection: invalid TLS setup");
	__ssl = SSL_new(ctx);
	if (!__ssl)
		throw std::runtime_error("admin connection: could not create TLS session");
	if (SSL_set_fd(__ssl, __sd) != 1)
	{
		SSL_free(__ssl);
		__ssl = NULL;
		throw std::runtime_error("admin connection: could not attach TLS session");
	}
	SSL_set_accept_state(__ssl);
	__tlsHandshake = true;
	__tlsOperation = TLS_OPERATION_HANDSHAKE;
	__tlsWait = TLS_WAIT_READ;
	__tlsFailed = false;
}

bool Connection::processTLSHandshake()
{
	if (!__tlsHandshake)
		return true;
	int result = SSL_accept(__ssl);
	if (result == 1)
	{
		__tlsHandshake = false;
		__tlsOperation = TLS_OPERATION_NONE;
		return true;
	}
	int error = SSL_get_error(__ssl, result);
	if (error == SSL_ERROR_WANT_WRITE)
		__tlsWait = TLS_WAIT_WRITE;
	else if (error == SSL_ERROR_WANT_READ)
		__tlsWait = TLS_WAIT_READ;
	else
	{
		__tlsFailed = true;
		return false;
	}
	return false;
}

bool Connection::usesTLS() const
{
	return __ssl != NULL;
}

bool Connection::tlsNeedsRead() const
{
	return !usesTLS() || __tlsOperation == TLS_OPERATION_NONE || __tlsWait == TLS_WAIT_READ;
}

bool Connection::tlsNeedsWrite() const
{
	if (!usesTLS())
		return false;
	if (__tlsOperation == TLS_OPERATION_NONE)
		return hasPendingOutput();
	return __tlsWait == TLS_WAIT_WRITE;
}

bool Connection::tlsReadPending() const
{
	return usesTLS() && __tlsOperation == TLS_OPERATION_READ;
}

bool Connection::tlsWritePending() const
{
	return usesTLS() && __tlsOperation == TLS_OPERATION_WRITE;
}

bool Connection::tlsHandshakePending() const
{
	return __tlsHandshake;
}

bool Connection::tlsHandshakeFailed() const
{
	return __tlsFailed;
}

ssize_t Connection::readSocket(char *buffer, size_t size)
{
	if (!usesTLS())
		return recv(__sd, buffer, size, 0);
	__tlsOperation = TLS_OPERATION_READ;
	int result = SSL_read(__ssl, buffer, static_cast<int>(size));
	if (result > 0)
	{
		__tlsOperation = TLS_OPERATION_NONE;
		return result;
	}
	int error = SSL_get_error(__ssl, result);
	if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
	{
		__tlsWait = (error == SSL_ERROR_WANT_WRITE) ? TLS_WAIT_WRITE : TLS_WAIT_READ;
		return -2;
	}
	if (error == SSL_ERROR_ZERO_RETURN)
	{
		__tlsOperation = TLS_OPERATION_NONE;
		return 0;
	}
	__tlsOperation = TLS_OPERATION_NONE;
	return -1;
}

ssize_t Connection::writeSocket(const char *buffer, size_t size)
{
	if (!usesTLS())
		return send(__sd, buffer, size, 0);
	__tlsOperation = TLS_OPERATION_WRITE;
	int result = SSL_write(__ssl, buffer, static_cast<int>(size));
	if (result > 0)
	{
		__tlsOperation = TLS_OPERATION_NONE;
		return result;
	}
	int error = SSL_get_error(__ssl, result);
	if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
	{
		__tlsWait = (error == SSL_ERROR_WANT_WRITE) ? TLS_WAIT_WRITE : TLS_WAIT_READ;
		return -2;
	}
	__tlsOperation = TLS_OPERATION_NONE;
	return -1;
}

void Connection::setSocket(int sd)
{
	this->__sd = sd;
	if (__server && __server->getServerType() == "game")
	{
		__connectionType = CONNECTION_GAME;
		__state = PLAYER_HANDSHAKE;
		__responseQueue.push(BasicString("BIENVENUE\n"));
	}
	else if (__server && __server->getServerType() == "admin")
	{
		__connectionType = CONNECTION_ADMIN;
	}
	else if (__server && __server->getServerType() == "gui")
	{
		__connectionType = CONNECTION_GUI;
	}
}

int Connection::getConnectionSocket()
{
	return __sd;
}

e_connection_type Connection::getConnectionType() const
{
	return __connectionType;
}

e_player_state Connection::getState() const
{
	return __state;
}

void Connection::setState(e_player_state state)
{
	__state = state;
}

int Connection::getPlayerId() const
{
	return __playerId;
}

void Connection::setPlayerId(int id)
{
	__playerId = id;
}

/*****************************************************************************
 *								MESSAGE FRAMING							     *
 *****************************************************************************/

void Connection::addData(const BasicString &input)
{
	this->__buffer.join(input);
}

void Connection::processMessage(const String &message)
{
	if (__connectionType == CONNECTION_GAME && __server)
		processGameMessage(message);
}

void Connection::processGameMessage(const String &message)
{
	GameServer *gs = static_cast<GameServer *>(__server);
	Game *game = gs->getGame();
	if (!game)
		return;

	if (__state == PLAYER_HANDSHAKE)
	{
		int playerId = -1;
		String response = game->handleHandshake(message, playerId);
		if (playerId >= 0)
		{
			__playerId = playerId;
			__state = PLAYER_ALIVE;
		}
		else
			__state = PLAYER_DEAD;
		__responseQueue.push(BasicString(response));
	}
	else if (__state == PLAYER_ALIVE && game->canAcceptCommand(__playerId))
		game->enqueueCommand(__playerId, message);
}

void Connection::processData()
{
	static const String delimiter(NEWLINE);

	while (!this->__buffer.empty())
	{
		size_t pos = this->__buffer.find(delimiter);
		if (pos == String::npos)
		{
			if (this->__buffer.length() > MAX_MESSAGE_SIZE)
				throw std::runtime_error("message exceeds maximum allowed size");
			return;
		}
		if (pos > MAX_MESSAGE_SIZE)
			throw std::runtime_error("message exceeds maximum allowed size");
		String message = this->__buffer.substr(0, pos).to_string();
		this->__buffer.erase(0, pos + delimiter.length());
		processMessage(message);
	}
	if (this->__buffer.length() > MAX_MESSAGE_SIZE)
		throw std::runtime_error("message exceeds maximum allowed size");
}

/*****************************************************************************
 *								OUTPUT QUEUE								 *
 *****************************************************************************/

bool Connection::hasPendingOutput() const
{
	return !this->__responseQueue.empty();
}

const BasicString &Connection::frontOutput() const
{
	return this->__responseQueue.front();
}

void Connection::popOutput()
{
	this->__responseQueue.pop();
	this->__responseOffset = 0;
}

size_t Connection::responseOffset() const
{
	return __responseOffset;
}

void Connection::setResponseOffset(size_t offset)
{
	__responseOffset = offset;
}

void Connection::pushOutput(const String &msg)
{
	__responseQueue.push(BasicString(msg));
}
