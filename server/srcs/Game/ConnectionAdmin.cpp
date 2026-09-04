#include "ConnectionAdmin.hpp"

ConnectionAdmin::ConnectionAdmin(Server *server) :    Connection(server),
                                            __responseOffset(0),
                                            __tlsHandshake(false),
                                            __tlsOperation(TLS_OPERATION_NONE),
                                            __tlsFailed(false),
                                            __tlsWait(TLS_WAIT_READ),
                                            __ssl(NULL)
{
	mzu::debug("ConnectionAdmin constructor");
}
ConnectionAdmin::ConnectionAdmin(const ConnectionAdmin &copy) : Connection(copy)
{
	mzu::debug("ConnectionAdmin copy constructor");
	*this = copy;
}
ConnectionAdmin &ConnectionAdmin::operator=(const ConnectionAdmin &assign)
{
	mzu::debug("ConnectionAdmin copy assignement operator");
	if (this != &assign)
	{
		__ssl = NULL;
		__tlsFailed = false;
		__tlsHandshake = false;
		__tlsWait = TLS_WAIT_READ;
		__tlsOperation = TLS_OPERATION_NONE;
		__responseOffset = assign.__responseOffset;
	}
	return *this;
}
ConnectionAdmin::~ConnectionAdmin()
{
	if (__ssl)
	{
		SSL_shutdown(__ssl);
		SSL_free(__ssl);
	}
	mzu::debug("ConnectionAdmin destructor");
}

void ConnectionAdmin::setupTLS(SSL_CTX *ctx)
{
	if (!ctx || __sd < 0)
		throw std::runtime_error("admin connectionAdmin: invalid TLS setup");
	__ssl = SSL_new(ctx);
	if (!__ssl)
		throw std::runtime_error("admin connectionAdmin: could not create TLS session");
	if (SSL_set_fd(__ssl, __sd) != 1)
	{
		SSL_free(__ssl);
		__ssl = NULL;
		throw std::runtime_error("admin connectionAdmin: could not attach TLS session");
	}
	SSL_set_accept_state(__ssl);
	__tlsHandshake = true;
	__tlsOperation = TLS_OPERATION_HANDSHAKE;
	__tlsWait = TLS_WAIT_READ;
	__tlsFailed = false;
}

bool ConnectionAdmin::processTLSHandshake()
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

bool ConnectionAdmin::usesTLS() const
{
	return __ssl != NULL;
}

bool ConnectionAdmin::tlsNeedsRead() const
{
	return !usesTLS() || __tlsOperation == TLS_OPERATION_NONE || __tlsWait == TLS_WAIT_READ;
}

bool ConnectionAdmin::tlsNeedsWrite() const
{
	if (!usesTLS())
		return false;
	if (__tlsOperation == TLS_OPERATION_NONE)
		return hasPendingOutput();
	return __tlsWait == TLS_WAIT_WRITE;
}

bool ConnectionAdmin::tlsReadPending() const
{
	return usesTLS() && __tlsOperation == TLS_OPERATION_READ;
}

bool ConnectionAdmin::tlsWritePending() const
{
	return usesTLS() && __tlsOperation == TLS_OPERATION_WRITE;
}

bool ConnectionAdmin::tlsHandshakePending() const
{
	return __tlsHandshake;
}

bool ConnectionAdmin::tlsHandshakeFailed() const
{
	return __tlsFailed;
}

bool ConnectionAdmin::readSocket()
{
    char buff[READ_SIZE + 1];
	if (this->tlsHandshakePending())
	{
		this->processTLSHandshake();
		if (this->tlsHandshakePending())
			return true;
		if (this->tlsHandshakeFailed())
			return false;
	}

	__tlsOperation = TLS_OPERATION_READ;
	int result = SSL_read(__ssl, buff, READ_SIZE);
	if (result > 0)
	{
		__tlsOperation = TLS_OPERATION_NONE;
        buff[result] = '\0';
        this->addData(BasicString(buff, result));
		return true;
	}
	int error = SSL_get_error(__ssl, result);
	if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
	{
		__tlsWait = (error == SSL_ERROR_WANT_WRITE) ? TLS_WAIT_WRITE : TLS_WAIT_READ;
		return true;
	}
	if (error == SSL_ERROR_ZERO_RETURN)
	{
		__tlsOperation = TLS_OPERATION_NONE;
		return false;
	}
	__tlsOperation = TLS_OPERATION_NONE;
	return false;
}

bool ConnectionAdmin::writeSocket()
{
	if (this->tlsHandshakePending())
	{
		this->processTLSHandshake();
		if (this->tlsHandshakePending())
			return true;
		if (this->tlsHandshakeFailed())
			return false;
	}

    if (!this->hasPendingOutput())
        return true;
    
    __tlsOperation = TLS_OPERATION_WRITE;
    const BasicString &out = this->frontOutput();
	int result = SSL_write(__ssl, out.getBuff() + this->responseOffset(), out.length() - this->responseOffset());
	if (result > 0)
	{
        mzu::info("response sent");
        __tlsOperation = TLS_OPERATION_NONE;
		if (static_cast<size_t>(result) + this->responseOffset() == out.length())
			this->popOutput();
		else
			this->setResponseOffset(this->responseOffset() + result);
		return true;
	}
	int error = SSL_get_error(__ssl, result);
	if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
	{
		__tlsWait = (error == SSL_ERROR_WANT_WRITE) ? TLS_WAIT_WRITE : TLS_WAIT_READ;
		return true;
	}
	__tlsOperation = TLS_OPERATION_NONE;
	return false;
}


/*****************************************************************************
 *								MESSAGE FRAMING							     *
 *****************************************************************************/

void ConnectionAdmin::processMessage(const String &message)
{
	mzu::info("received: \"" + message + "\"");
}

void ConnectionAdmin::popOutput()
{
	this->__responseQueue.pop();
    this->__responseOffset = 0;
}

size_t ConnectionAdmin::responseOffset() const
{
	return __responseOffset;
}

void ConnectionAdmin::setResponseOffset(size_t offset)
{
	__responseOffset = offset;
}
