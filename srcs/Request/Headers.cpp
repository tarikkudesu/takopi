#include "Headers.hpp"

Headers::Headers() : __contentType(OTHER),
					 __contentLength(0),
					 __connectionType(KEEP_ALIVE),
					 __transferType(NONE)
{
	wsu::debug("Headers default constructor");
}
Headers::Headers(const Headers &copy)
{
	wsu::debug("Headers copy constructor");
	*this = copy;
}
Headers &Headers::operator=(const Headers &assign)
{
	wsu::debug("Headers copy assignement operator");
	if (this != &assign)
	{
		this->__host = assign.__host;
		this->__port = assign.__port;
		this->__cookie = assign.__cookie;
		this->__boundary = assign.__boundary;
		this->__contentType = assign.__contentType;
		this->__contentLength = assign.__contentLength;
		this->__connectionType = assign.__connectionType;
		this->__transferEncoding = assign.__transferEncoding;
	}
	return *this;
}
Headers::~Headers()
{
	wsu::debug("Headers destructor");
}
/***********************************************************************************************
 *											 METHODS										   *
 ***********************************************************************************************/

void Headers::clear()
{
	this->__port = 8080;
	this->__host.clear();
	this->__cookie.clear();
	this->__boundary.clear();
	this->__contentLength = 0;
	this->__contentType = OTHER;
	this->__transferEncoding.clear();
	this->__connectionType = KEEP_ALIVE;
}
String Headers::getHeaderFeildValue(const String &key, Map &headers)
{
	Map::iterator iter = headers.find(key);
	if (iter == headers.end())
		throw std::exception();
	return iter->second;
}
void Headers::contentLength(Map &headers)
{
	try
	{
		String value = getHeaderFeildValue("CONTENT-LENGTH", headers);
		this->__contentLength = wsu::stringToInt(value);
		if (this->__contentLength != 0)
			this->__transferType = DEFINED;
	}
	catch (std::exception &e)
	{
	}
}
void Headers::contentType(Map &headers)
{
	try
	{
		String contentType = getHeaderFeildValue("CONTENT-TYPE", headers);
		t_svec elements = wsu::splitByChar(contentType, ';');
		if (elements.size() == 1 && elements.at(0) == FORM_DATA)
			this->__contentType = FORM;
		if (elements.size() == 2 && elements.at(0) == MULTIPART_DATA_FORM)
		{
			wsu::trimSpaces(elements.at(1));
			t_svec tmp = wsu::splitByChar(elements.at(1), '=');
			if (tmp.size() == 2)
			{
				__boundary = tmp.at(1);
				this->__contentType = MULTIPART;
			}
		}
	}
	catch (std::exception &e)
	{
	}
}

void Headers::connectionType(Map &headers)
{
	try
	{
		String value = getHeaderFeildValue("CONNECTION", headers);
		if (value == "close")
			this->__connectionType = CLOSE;
	}
	catch (std::exception &e)
	{
	}
}
void Headers::cookie(Map &headers)
{
	try
	{
		this->__cookie = getHeaderFeildValue("COOKIE", headers);
	}
	catch (std::exception &e)
	{
	}
}
void Headers::transferEncoding(Map &headers)
{
	try
	{
		this->__transferEncoding = getHeaderFeildValue("TRANSFER-ENCODING", headers);
		if (this->__transferEncoding.find("chunked") != String::npos)
			this->__transferType = CHUNKED;
	}
	catch (std::exception &e)
	{
	}
}
void Headers::range(Map &headers)
{
	try
	{
		getHeaderFeildValue("RANGE", headers);
		throw ErrorResponse(416);
	}
	catch (std::exception &e)
	{
	}
}
void Headers::hostAndPort(Map &headers)
{
	try
	{
		String value = getHeaderFeildValue("HOST", headers);
		size_t pos = value.find(":");
		if (pos == String::npos)
		{
			this->__host = value;
			this->__port = 8080;
		}
		else
		{
			this->__host = String(value.begin(), value.begin() + pos);
			String port = String(value.begin() + pos + 1, value.end());
			this->__port = std::strtol(port.c_str(), NULL, 10);
		}
	}
	catch (std::exception &e)
	{
		throw ErrorResponse(400);
	}
}
void Headers::parseHeaders(Map &headers)
{
	clear();
	hostAndPort(headers);
	connectionType(headers);
	contentLength(headers);
	transferEncoding(headers);
	contentType(headers);
	cookie(headers);
}

std::ostream &operator<<(std::ostream &o, const Headers &obj)
{
	o << "\t\tPort: " << obj.__port << std::endl;
	o << "\t\tHost: " << obj.__host << std::endl;
	o << "\t\tContent-Type: " << obj.__contentType << std::endl;
	o << "\t\tContent-Length: " << obj.__contentLength << std::endl;
	o << "\t\tTransfer-Encoding: " << obj.__transferEncoding << "\n";
	o << "\t\tCookie: " << obj.__cookie << std::endl;
	return o;
}
