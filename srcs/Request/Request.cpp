#include "Request.hpp"

Request::Request(t_connection_phase &phase) : __connectionPhase(phase),
											  __startTime(std::time(NULL)),
											  __bodySize(0),
											  __requestPhase(REQUEST_INIT)
{
	wsu::debug("Request default constructor");
}
Request::Request(const Request &copy) : __connectionPhase(copy.__connectionPhase)
{
	wsu::debug("Request copy constructor");
	*this = copy;
}
Request &Request::operator=(const Request &assign)
{
	wsu::debug("Request copy assignement operator");
	if (this != &assign)
	{
		this->__URI = assign.__URI;
		this->__method = assign.__method;
		this->__headers = assign.__headers;
		this->__bodySize = assign.__bodySize;
		this->__fragement = assign.__fragement;
		this->__startTime = assign.__startTime;
		this->__protocole = assign.__protocole;
		this->__queryString = assign.__queryString;
		this->__requestLine = assign.__requestLine;
		this->__headerFeilds = assign.__headerFeilds;
		this->__requestPhase = assign.__requestPhase;
		this->__requestHeaders = assign.__requestHeaders;
	}
	return *this;
}
Request::~Request()
{
	wsu::debug("Request destructor");
}

/****************************************************************************
 *								 MINI METHODS								*
 ****************************************************************************/

void Request::reset()
{
	this->__URI.clear();
	this->__bodySize = 0;
	this->__headers.clear();
	this->__fragement.clear();
	this->__protocole.clear();
	this->__requestLine.clear();
	this->__queryString.clear();
	this->__headerFeilds.clear();
	this->__headerFeilds.clear();
	this->__requestHeaders.clear();
}

/*****************************************************************************
 *									METHODS 								 *
 *****************************************************************************/

void Request::validateHeaders()
{
	size_t pos = 0;
	do
	{
		pos = __requestHeaders.find("\r\n");
		if (pos == String::npos)
			break;
		{
			String hf(__requestHeaders.begin(), __requestHeaders.begin() + pos);
			size_t p = hf.find(": ");
			if (p == String::npos)
				throw ErrorResponse(400);
			String key(hf.begin(), hf.begin() + p);
			wsu::toUpperString(key);
			String value(hf.begin() + p + 2, hf.end());
			if (key.empty() || String::npos != key.find_first_not_of(H_KEY_CHAR_SET))
				throw ErrorResponse(400);
			wsu::trimSpaces(key);
			wsu::trimSpaces(value);
			this->__headerFeilds[key] = value;
			__requestHeaders.erase(0, pos + 2);
		}
	} while (__requestHeaders.empty() == false);
}
void Request::validateURI()
{
	size_t start = 0;
	size_t end = 0;
	start = this->__URI.find("?");
	end = this->__URI.find("#");
	if (end != String::npos)
	{
		this->__fragement = String(this->__URI.begin() + end + 1, this->__URI.end());
		this->__URI.erase(end);
	}
	if (start != String::npos)
	{
		this->__queryString = String(this->__URI.begin() + start + 1, this->__URI.end());
		this->__URI.erase(start);
	}
}
void Request::validateRequestLine()
{
	if (2 < std::count(__requestLine.begin(), __requestLine.end(), ' '))
		throw ErrorResponse(400);
	std::istringstream iss(__requestLine);
	String method, URI, protocole;
	iss >> method;
	iss >> URI;
	iss >> protocole;
	if (method.empty() || protocole.empty() || URI.empty())
		throw ErrorResponse(400);
	if (method == "OPTIONS")
		this->__method = OPTIONS;
	else if (method == "GET")
		this->__method = GET;
	else if (method == "HEAD")
		this->__method = HEAD;
	else if (method == "POST")
		this->__method = POST;
	else if (method == "PUT")
		this->__method = PUT;
	else if (method == "DELETE")
		this->__method = DELETE;
	else if (method == "TRACE")
		this->__method = TRACE;
	else if (method == "CONNECT")
		this->__method = CONNECT;
	else
		throw ErrorResponse(501);
	this->__URI = wsu::decode(URI);
	this->__protocole = protocole;
	if (this->__protocole != PROTOCOLE_V)
		throw ErrorResponse(505);
	validateURI();
}
void Request::parseRequest()
{
	size_t h = __data.find(D_LINE_BREAK);
	size_t s = __data.find(LINE_BREAK);
	h -= (s + 2);
	__requestLine = __data.substr(0, s).to_string();
	__data.erase(0, s + 2);
	__requestHeaders = __data.substr(0, h + 2).to_string();
	__data.erase(0, h + 4);
	validateRequestLine();
	validateHeaders();
	__headers.parseHeaders(__headerFeilds);
}
void Request::requestInit()
{
	reset();
	__startTime = std::time(NULL);
	__requestPhase = REQUEST_PARSE;
}
void Request::requestExecute(BasicString &data)
{
	size_t h = data.find(D_LINE_BREAK);
	if (h == String::npos)
	{
		__data.join(data);
		if (__data.length() > REQUEST_MAX_SIZE)
		{
			__data.clear();
			throw ErrorResponse(400);
		}
		if (std::time(NULL) - this->__startTime > CLIENT_TIMEOUT)
		{
			__data.clear();
			throw ErrorResponse(408);
		}
		data.clear();
		throw wsu::persist();
	}
	__data.join(data.substr(0, h + 4));
	data.erase(0, h + 4);
	parseRequest();
	this->__connectionPhase = IDENTIFY_WORKERS;
	this->__requestPhase = REQUEST_INIT;
}
void Request::processData(BasicString &data)
{
	wsu::debug("processing request");
	if (__requestPhase == REQUEST_INIT)
		requestInit();
	if (__requestPhase == REQUEST_PARSE)
		requestExecute(data);
}
