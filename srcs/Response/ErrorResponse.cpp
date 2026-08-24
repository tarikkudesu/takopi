#include "ErrorResponse.hpp"

ErrorResponse::ErrorResponse(int code) : __code(code),
										 __location(NULL)
{
	wsu::debug("ErrorResponse default constructor");
	this->constructErrorPage();
}
ErrorResponse::ErrorResponse(int code, Location &location) : __code(code),
															 __location(&location)
{
	wsu::debug("ErrorResponse para constructor");
	this->constructErrorPage();
}
ErrorResponse::ErrorResponse(int code, String redirection, Location &location) : __code(code),
																				 __location(&location),
																				 __redirection(redirection)
{
	wsu::debug("ErrorResponse para constructor");
	this->constructErrorPage();
}
ErrorResponse::ErrorResponse(int code, String redirection, String cookie, Location &location) : __code(code),
																								__cookie(cookie),
																								__location(&location),
																								__redirection(redirection)
{
	wsu::debug("ErrorResponse para constructor");
	this->constructErrorPage();
}
ErrorResponse::ErrorResponse(const ErrorResponse &copy)
{
	wsu::debug("ErrorResponse copy constructor");
	*this = copy;
}
ErrorResponse &ErrorResponse::operator=(const ErrorResponse &assign)
{
	wsu::debug("ErrorResponse copy assignement operator");
	if (this != &assign)
	{
		this->__page = assign.__page;
		this->__code = assign.__code;
		this->__Body = assign.__Body;
		this->__headers = assign.__headers;
		this->__location = assign.__location;
		this->__StatusLine = assign.__StatusLine;
		this->__redirection = assign.__redirection;
		this->__reasonPhrase = assign.__reasonPhrase;
	}
	return *this;
}
ErrorResponse::~ErrorResponse()
{
	wsu::debug("ErrorResponse destructor");
}
/****************************************************************************
 *								 MINI METHODS								*
 ****************************************************************************/

BasicString ErrorResponse::getResponse() const
{
	return this->__StatusLine + LINE_BREAK + this->__headers + this->__Body;
}

/*****************************************************************************
 *									METHODS 								 *
 *****************************************************************************/

void ErrorResponse::buildResponseBody()
{
	if (!this->__redirection.empty())
		return;
	try
	{
		if (this->__page.empty())
			throw std::runtime_error("");
		std::fstream fS;
		String line;
		fS.open(this->__page.c_str());
		if (!fS.is_open())
			throw std::runtime_error("");
		do
		{
			std::getline(fS, line, '\n');
			this->__Body += line;
			if (fS.eof())
				break;
			line.clear();
		} while (true);
		fS.close();
	}
	catch (std::exception &e)
	{
		this->__Body = ErrorResponse::__errPage;
		wsu::replaceString(this->__Body, "CODE", wsu::intToString(this->__code));
		wsu::replaceString(this->__Body, "REASON_PHRASE", this->__reasonPhrase);
	}
}
void ErrorResponse::buildStatusLine()
{
	this->__StatusLine = PROTOCOLE_V " " + wsu::intToString(this->__code) + " " + this->__reasonPhrase;
}
void ErrorResponse::buildHeaderFeilds()
{
	this->__headers += "Accept-Ranges: none" LINE_BREAK;
	this->__headers += "Connection: keep-alive" LINE_BREAK;
	this->__headers += "Content-Length: " + wsu::intToString(this->__Body.length()) + "" LINE_BREAK;
	this->__headers += "Content-Type: " + wsu::getContentType(this->__page) + "; charset=UTF-8" LINE_BREAK;
	this->__headers += "Cache-Control: no-store, no-cache, must-revalidate" LINE_BREAK;
	this->__headers += "Pragma: no-cache" LINE_BREAK;
	this->__headers += "Server: Webserv" LINE_BREAK;
	this->__headers += "Date: " + wsu::buildIMFDate(0) + "" LINE_BREAK;
	if (!this->__redirection.empty())
		this->__headers += "Location: " + this->__redirection + "" LINE_BREAK;
	if (!this->__cookie.empty())
		this->__headers += "Set-Cookie: token=" + this->__cookie + "; path=/; expires=" + wsu::buildIMFDate(3600) + ";" LINE_BREAK;
	this->__headers += LINE_BREAK;
}
void ErrorResponse::constructErrorPage()
{
	std::map<int16_t, String>::iterator it = wsu::__errCode.find(this->__code);
	if (it != wsu::__errCode.end())
		this->__reasonPhrase = it->second;
	else
		this->__reasonPhrase = "Error";
	if (__location)
	{
		std::map<int16_t, String>::iterator it = __location->__errorPages.find(this->__code);
		if (it != __location->__errorPages.end())
			this->__page = __location->__errorPages[this->__code];
	}
	if (this->__page.empty())
	{
		std::map<int16_t, String>::iterator it = wsu::__defaultErrorPages.find(this->__code);
		if (it != __location->__errorPages.end())
			this->__page = it->second;
	}
	buildStatusLine();
	buildResponseBody();
	buildHeaderFeilds();
}
/***********************************************************************
 *							  STATIC METHODS						   *
 ***********************************************************************/

String ErrorResponse::__errPage = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">\n"
								  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
								  "<title>Webserv</title></head><body style=\"background-color: rgb(35, 40, 47);\">\n"
								  "<div style=\"border: 1px solid rgba(210, 215, 223, 0.26); border-radius: 4px; margin-top: 100px; background-color: rgb(22, 27, 34);\">\n"
								  "<h1 style=\"font-size: 80px; font-family: sans-serif; text-align: center; padding: 20px 0px 0px 0px; margin: 0px; color: rgb(210, 215, 223);\">\n"
								  "CODE\n"
								  "</h1><h2 style=\"font-size: 25px; font-family: sans-serif; text-align: center; padding: 0px 0px 20px 0px; margin: 0px; color: rgb(210, 215, 223);\">\n"
								  "REASON_PHRASE\n"
								  "</h2></div></body></html>";
