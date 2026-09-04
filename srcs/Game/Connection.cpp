#include "Connection.hpp"
#include "../ServerManager/ServerGame.hpp"
#include "../Game/Game.hpp"

Connection::Connection(Server *server) :	__sd(-1),
											__type(server->getType()),
											__server(server)
{
	mzu::debug("Connection constructor");
}
Connection::Connection(const Connection &copy) : 	__sd(-1),
													__server(NULL)
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
		__type = assign.__type;
		__server = assign.__server;
		__responseQueue = assign.__responseQueue;
	}
	return *this;
}
Connection::~Connection()
{
	mzu::debug("Connection destructor");
}

void Connection::setSocket(int sd)
{
	this->__sd = sd;
}

int Connection::getConnectionSocket()
{
	return __sd;
}

e_type	Connection::getType() const
{
	return __type;
}

/*****************************************************************************
 *								MESSAGE FRAMING							     *
 *****************************************************************************/

void Connection::addData(const BasicString &input)
{
	this->__buffer.join(input);
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
}

void Connection::pushOutput(const String &msg)
{
	__responseQueue.push(BasicString(msg));
}
