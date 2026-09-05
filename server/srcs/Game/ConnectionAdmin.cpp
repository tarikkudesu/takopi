#include "ConnectionAdmin.hpp"
#include "../ServerManager/Core.hpp"
#include "../ServerManager/ServerAdmin.hpp"

ConnectionAdmin::ConnectionAdmin(Server *server) :	Connection(server),
													__ssl(NULL),
													__tlsWait(TLS_WAIT_READ),
													__tlsOperation(TLS_OPERATION_NONE),
													__authenticationFailures(0),
													__responseOffset(0),
													__authenticated(false),
													__tlsHandshake(false),
													__tlsFailed(false),
													__closing(false)
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
		__closing = assign.__closing;
		__tlsOperation = TLS_OPERATION_NONE;
		__responseOffset = assign.__responseOffset;
		__authenticated = assign.__authenticated;
		__authenticationFailures = assign.__authenticationFailures;
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
	__tlsOperation = TLS_OPERATION_HANDSHAKE;
	__tlsWait = TLS_WAIT_READ;
	__tlsHandshake = true;
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

	if (__closing && !this->hasPendingOutput())
		return false;
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
    ServerAdmin *server = static_cast<ServerAdmin *>(this->__server);
    String command = message;

    if (!command.empty() && command[command.length() - 1] == '\r')
        command.erase(command.length() - 1);

    if (this->__closing)
        return;

    if (!this->__authenticated)
    {
        if (command.find("AUTH ") == 0)
        {
            String password = command.substr(5);

            if (server->checkPassword(password))
            {
                this->__authenticated = true;
                this->__authenticationFailures = 0;
                mzu::info("remote administrator authenticated");
                this->pushOutput( ADMIN_OK "Authentication successful.\n" ADMIN_PROMPT );
                return;
            }
            this->__authenticationFailures++;
            mzu::warn("remote administrator authentication failed");
            if (this->__authenticationFailures >= MAX_ADMIN_AUTH_FAILURES)
            {
                this->pushOutput( ADMIN_ERR "Too many authentication failures.\n" RED "      Connection closed." RESET "\n" );
                this->__closing = true;
                return;
            }
            this->pushOutput( ADMIN_ERR "Invalid password.\n" YELLOW "      Usage:" RESET " AUTH <password>\n" );
            return;
        }
        this->pushOutput( ADMIN_WARN "Authentication required.\n" YELLOW "       Usage:" RESET " AUTH <password>\n" );
        return;
    }
    if (command.empty())
    {
        this->pushOutput(ADMIN_PROMPT);
        return;
    }
    if (command.find("AUTH ") == 0)
    {
        mzu::warn("authentication command ignored for authenticated administrator");
        this->pushOutput( ADMIN_WARN "Already authenticated.\n" ADMIN_PROMPT );
        return;
    }
    mzu::running("[admin remote] command received: " + command);
    String response = Core::executeAdminCommand(command);
    if (!response.empty())
        this->pushOutput(response);
    this->pushOutput(ADMIN_PROMPT);
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
