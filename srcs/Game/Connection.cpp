#include "Connection.hpp"

Connection::Connection(Server *server) : __sd(-1)
{
	mzu::debug("Connection constructor");
}
Connection::Connection(const Connection &copy)
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

	if (this->__buffer.empty())
		return;
	size_t pos = this->__buffer.find(delimiter);
	if (pos == String::npos)
		return;
	if (pos > MAX_MESSAGE_SIZE)
		throw std::runtime_error("message exceeds maximum allowed size");
	String message = this->__buffer.substr(0, pos).to_string();
	this->__buffer.erase(0, pos + delimiter.length());
	mzu::info("received: \"" + message + "\"");
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
