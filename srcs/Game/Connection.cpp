#include "Connection.hpp"

Connection::Connection(Server *server) : __sd(-1),
										 __server(server)
{
	wsu::debug("Connection constructor");
}
Connection::~Connection()
{
	wsu::debug("Connection destructor");
}

void Connection::setSocket(int sd)
{
	this->__sd = sd;
}
void Connection::setServer(Server *server)
{
	this->__server = server;
}
int Connection::getConnectionSocket()
{
	return __sd;
}

/*****************************************************************************
 *									MESSAGE FRAMING							 *
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
			break;
		if (pos > MAX_MESSAGE_SIZE)
			throw std::runtime_error("message exceeds maximum allowed size");
		String message = this->__buffer.substr(0, pos).to_string();
		this->__buffer.erase(0, pos + delimiter.length());
		if (!message.empty() && *(message.end() - 1) == '\r')
			message.erase(message.size() - 1);
		if (this->__server != NULL && !message.empty())
			this->__client.handleMessage(message);
	}
	if (this->__buffer.length() > MAX_MESSAGE_SIZE)
		throw std::runtime_error("message exceeds maximum allowed size");
}

/*****************************************************************************
 *									OUTPUT QUEUE								 *
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
