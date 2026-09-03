#include "Connection.hpp"

Connection::Connection() : __sd(-1), __outOffset(0)
{
	mzu::debug("Connection constructor");
}
Connection::Connection(const Connection &copy) : __sd(-1), __outOffset(0)
{
	mzu::debug("Connection copy constructor");
	*this = copy;
}
Connection &Connection::operator=(const Connection &assign)
{
	mzu::debug("Connection copy assignement operator");
	if (this != &assign)
	{
		__sd = assign.__sd;
		__inBuffer = assign.__inBuffer;
		__outOffset = assign.__outOffset;
	}
	return *this;
}
Connection::~Connection()
{
	if (__sd != -1)
		close(__sd);
	mzu::debug("Connection destructor");
}

/****************************************************************************
 *								SOCKET SETUP								*
 ****************************************************************************/

void Connection::connectTo(const String &host, int port)
{
	struct addrinfo hints;
	struct addrinfo *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host.c_str(), mzu::intToString(port).c_str(), &hints, &result) != 0)
		throw std::runtime_error("could not resolve host: " + host);

	__sd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (__sd == -1)
	{
		freeaddrinfo(result);
		throw std::runtime_error("failed to create socket");
	}
	if (connect(__sd, result->ai_addr, result->ai_addrlen) == -1)
	{
		freeaddrinfo(result);
		close(__sd);
		__sd = -1;
		throw std::runtime_error("failed to connect to " + host + ":" + mzu::intToString(port));
	}
	freeaddrinfo(result);
	if (fcntl(__sd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(__sd);
		__sd = -1;
		throw std::runtime_error("fcntl syscall, failed to make a non blocking socket");
	}
}

int Connection::getSocket() const
{
	return __sd;
}

/****************************************************************************
 *								MESSAGE FRAMING								*
 ****************************************************************************/

bool Connection::readSocket()
{
	char buff[READ_SIZE + 1];

	ssize_t bytesRead = recv(__sd, buff, READ_SIZE, 0);
	if (bytesRead == 0)
		return false;
	if (bytesRead < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;
		return false;
	}
	buff[bytesRead] = '\0';
	__inBuffer.join(BasicString(buff, static_cast<size_t>(bytesRead)));
	extractLines();
	return true;
}

void Connection::extractLines()
{
	static const String delimiter(NEWLINE);

	while (!__inBuffer.empty())
	{
		size_t pos = __inBuffer.find(delimiter);
		if (pos == String::npos)
		{
			if (__inBuffer.length() > MAX_MESSAGE_SIZE)
				throw std::runtime_error("message exceeds maximum allowed size");
			return;
		}
		if (pos > MAX_MESSAGE_SIZE)
			throw std::runtime_error("message exceeds maximum allowed size");
		__inbox.push(__inBuffer.substr(0, pos).to_string());
		__inBuffer.erase(0, pos + delimiter.length());
	}
}

bool Connection::hasInboundLine() const
{
	return !__inbox.empty();
}

String Connection::popInboundLine()
{
	String line = __inbox.front();
	__inbox.pop();
	return line;
}

/****************************************************************************
 *								OUTPUT QUEUE								*
 ****************************************************************************/

bool Connection::hasPendingOutput() const
{
	return !__outQueue.empty();
}

void Connection::pushOutput(const String &line)
{
	__outQueue.push(BasicString(line + NEWLINE));
}

bool Connection::writeSocket()
{
	if (__outQueue.empty())
		return true;

	const BasicString &out = __outQueue.front();
	ssize_t bytesWritten = send(__sd, out.getBuff() + __outOffset, out.length() - __outOffset, 0);
	if (bytesWritten < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;
		return false;
	}
	__outOffset += static_cast<size_t>(bytesWritten);
	if (__outOffset >= out.length())
	{
		__outQueue.pop();
		__outOffset = 0;
	}
	return true;
}
