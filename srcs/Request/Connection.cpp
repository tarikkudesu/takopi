#include "Connection.hpp"

Connection::Connection() : __sd(-1),
						   __phase(PROCESSING_REQUEST),
						   __request(__phase),
						   __response(__phase, __request),
						   __serversP(NULL)
{
	wsu::debug("Connection default constructor");
}
Connection::Connection(int sd) : __sd(sd),
								 __phase(PROCESSING_REQUEST),
								 __request(__phase),
								 __response(__phase, __request),
								 __serversP(NULL)
{
	wsu::debug("Connection single para constructor");
}
Connection::Connection(const Connection &copy) : __request(copy.__request),
												 __response(copy.__response)
{
	wsu::debug("Connection copy constructor");
	*this = copy;
}
Connection &Connection::operator=(const Connection &assign)
{
	wsu::debug("Connection copy assignement operator");
	if (this != &assign)
	{
		__sd = assign.__sd;
		__phase = assign.__phase;
		__serversP = assign.__serversP;
		__responseQueue = assign.__responseQueue;
	}
	return *this;
}
Connection::~Connection()
{
	wsu::debug("Connection destructor");
}

void Connection::setSocket(int sd)
{
	this->__sd = sd;
}
void Connection::setServers(t_Server &servers)
{
	this->__serversP = &servers;
}
int Connection::getConnectionSocket()
{
	return __sd;
}
bool Connection::close()
{
	return __request.__headers.__connectionType == CLOSE ? true : false;
}
/*****************************************************************************
 *									METHODS 								 *
 *****************************************************************************/
Server *Connection::identifyServer()
{
	t_serVect tmpMapP, tmpMapH;
	for (t_Server::iterator it = this->__serversP->begin(); it != this->__serversP->end(); it++)
	{
		if (it->second->getServerPort() == this->__request.__headers.__port)
			tmpMapP.push_back(it->second);
	}
	for (t_serVect::iterator it = tmpMapP.begin(); it != tmpMapP.end(); it++)
	{
		if ((*it)->amITheServerYouAreLookingFor(this->__request.__headers.__host) == true)
			tmpMapH.push_back(*it);
	}
	if (tmpMapH.empty() && tmpMapP.empty())
		return this->__serversP->begin()->second;
	if (tmpMapH.empty())
		return tmpMapP.at(0);
	return tmpMapH.at(0);
}
void Connection::identifyWorkers()
{
	wsu::debug("identifying workers");
	Server *server = identifyServer();
	Location &location = server->identifyLocation(__request.__URI);
	__response.setupWorkers(*server, location);
	this->__phase = PROCESSING_RESPONSE;
}
/**********************************************************************************
 *									PROCESS DATA								  *
 **********************************************************************************/

void Connection::addData(const BasicString &input)
{
	wsu::info("receiving data");
	this->__data.join(input);
}

void Connection::processData()
{
	// an important thing to note here is that phases will be removed as the requests and responses will be small an emidiate, so it will simpler if they were simple queues
	if (__phase == PROCESSING_REQUEST && __request.__requestPhase == REQUEST_INIT && __data.empty())
		return;
	try
	{
		wsu::debug("processing data");
		if (__phase == PROCESSING_REQUEST)
			__request.processData(__data);
		if (__phase == IDENTIFY_WORKERS)
			this->identifyWorkers();
		if (__phase == PROCESSING_RESPONSE)
			__response.processData(__data);
		if (!__response.getResponse().empty())
			this->__responseQueue.push(__response.getResponse());
	}
	catch (ErrorResponse &e)
	{
		this->__responseQueue.push(e.getResponse());
		wsu::info("request: " + wsu::methodToString(__request.__method) + " " + __request.__URI + " " + PROTOCOLE_V);
		wsu::info("response: " + e.__StatusLine);
		this->__phase = PROCESSING_REQUEST;
		this->__response.reset();
		this->__request.reset();
	}
	catch (wsu::persist &e)
	{
	}
}
