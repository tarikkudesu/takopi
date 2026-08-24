#include "Response.hpp"

Response::Response(t_connection_phase &phase, Request &request) : __connectionPhase(phase),
																  __responsePhase(PREPARING_RESPONSE),
																  __postPhase(POST_INIT),
																  __getPhase(GET_INIT),
																  __get(request, __responsePhase),
																  __post(request, __responsePhase),
																  __request(request)
{
	wsu::debug("Response default constructor");
}
Response::Response(const Response &copy) : __connectionPhase(copy.__connectionPhase),
										   __responsePhase(copy.__responsePhase),
										   __postPhase(copy.__postPhase),
										   __getPhase(copy.__getPhase),
										   __get(copy.__get),
										   __post(copy.__post),
										   __request(copy.__request)
{
	wsu::debug("Response copy constructor");
	*this = copy;
}
Response &Response::operator=(const Response &assign)
{
	wsu::debug("Response copy assignement operator");
	if (this != &assign)
	{
		this->__get = assign.__get;
		this->__body = assign.__body;
		this->__post = assign.__post;
		this->__server = assign.__server;
		this->__request = assign.__request;
		this->__location = assign.__location;
		this->__explorer = assign.__explorer;
		this->__getPhase = assign.__getPhase;
		this->__postPhase = assign.__postPhase;
		this->__responsePhase = assign.__responsePhase;
		this->__connectionPhase = assign.__connectionPhase;
	}
	return *this;
}
Response::~Response()
{
	wsu::debug("Response destructor");
	for (t_svec::iterator it = this->__tempFiles.begin(); it != this->__tempFiles.end(); it++)
		unlink(it->c_str());
	this->__tempFiles.clear();
}
/***********************************************************************
 *								 METHODS							   *
 ***********************************************************************/

void Response::reset()
{
	__get.reset();
	__post.reset();
	__getPhase = GET_INIT;
	__postPhase = POST_INIT;
	__responsePhase = PREPARING_RESPONSE;
	__connectionPhase = PROCESSING_REQUEST;
	t_svec temp;
	for (t_svec::iterator it = this->__tempFiles.begin(); it != this->__tempFiles.end(); it++)
	{
		if (std::time(NULL) - wsu::getFileLastModifiedTime(*it) > TEMP_FILE_LIFE_SPAN)
		{
			temp.push_back(it->c_str());
			unlink(it->c_str());
		}
	}
	for (t_svec::iterator it = temp.begin(); it != temp.end(); it++)
	{
		t_svec::iterator iter = std::find(__tempFiles.begin(), __tempFiles.end(), *it);
		if (iter != __tempFiles.end())
			this->__tempFiles.erase(iter);
	}
	temp.clear();
}
BasicString Response::getResponse()
{
	return __body;
}
bool Response::shouldAuthenticate()
{
	if (__location->__authenticate.size() == 2)
		return __explorer.__fullPath == wsu::joinPaths(__location->__root, __location->__authenticate[0]);
	return false;
}
void Response::buildResponse(int code, ssize_t length)
{
	__body.clear();
	String reasonPhrase;
	std::map<int16_t, String>::iterator it = wsu::__errCode.find(code);
	if (it != wsu::__errCode.end())
		reasonPhrase = it->second;
	else
		reasonPhrase = "OK";
	__body.join(PROTOCOLE_V " " + wsu::intToString(code) + " " + reasonPhrase + LINE_BREAK);
	__body.join("Content-Type: " + wsu::getContentType(__explorer.__fullPath) + "; charset=UTF-8" + LINE_BREAK);
	__body.join("Cache-Control: no-store, no-cache, must-revalidate" + String(LINE_BREAK));
	__body.join("Pragma: no-cache" + String(LINE_BREAK));
	__body.join("date: " + wsu::buildIMFDate(0) + LINE_BREAK);
	__body.join(String("Accept-Ranges: none") + LINE_BREAK);
	__body.join(String("server: webserv/1.0") + LINE_BREAK);
	if (__request.__headers.__connectionType == CLOSE)
		__body.join(String("Connection: close") + LINE_BREAK);
	else
		__body.join(String("Connection: keep-alive") + LINE_BREAK);
	if (length != -1)
		__body.join("Content-Length: " + wsu::intToString(length) + LINE_BREAK);
	__body.join(String(LINE_BREAK));

	wsu::info("request: " + wsu::methodToString(__request.__method) + " " + __request.__URI + " " + PROTOCOLE_V);
	wsu::info("response: " PROTOCOLE_V " " + wsu::intToString(code) + " " + reasonPhrase);
}
bool Response::checkCgi()
{
	if (__location->__cgiPass.empty())
		return 0;
	if (__explorer.__type == FOLDER)
		return 0;
	if (wsu::endWith(__explorer.__fullPath, ".java") || wsu::endWith(__explorer.__fullPath, ".php"))
		return 1;
	return 0;
}
void Response::__check_methods()
{
	if (__request.__method == GET)
		__responsePhase = GET_PROCESS;
	else if (__request.__method == POST)
		__responsePhase = POST_PROCESS;
	else if (__request.__method == DELETE)
		__responsePhase = DELETE_PROCESS;
	else
		throw ErrorResponse(405, *__location);
}

void Response::setupWorkers(Server &server, Location &location)
{
	this->__server = &server;
	this->__location = &location;
}

/*************************************************************************************
 *									BODY PROCESSING 								 *
 *************************************************************************************/

void Response::postDone()
{
	String form = __post.getForm().to_string();
	form = wsu::decode(form);
	if (__location->__authenticate.size() == 2 &&
		__explorer.__fullPath == wsu::joinPaths(__location->__root, __location->__authenticate[1]))
	{
		String cookie = __server->userInDb(form, 1);
		if (cookie.empty())
			cookie = __server->addUserInDb(form);
		throw ErrorResponse(302, __location->__authenticate[0], cookie, *__location);
	}
	else if (__request.__headers.__contentType == MULTIPART)
	{
		String b = wsu::readFielContent(SUCCESS_PAGE_PATH);
		buildResponse(201, b.length());
		__body.join(b);
		__responsePhase = RESPONSE_DONE;
	}
	else
		__responsePhase = CGI_PROCESS;
}
void Response::processCunkedBody(BasicString &data)
{
	static size_t chunkSize;
	do
	{
		if (chunkSize == 0)
		{
			size_t pos = data.find(LINE_BREAK);
			if (pos == String::npos && data.length() > REQUEST_MAX_SIZE)
			{
				data.clear();
				__request.__headers.__connectionType = CLOSE;
				throw ErrorResponse(400, *__location);
			}
			else if (pos == String::npos)
				throw wsu::persist();
			BasicString hex = data.substr(0, pos);
			size_t extPos = hex.find(";");
			if (extPos != String::npos)
				hex = hex.substr(0, extPos);
			chunkSize = wsu::hexToInt(hex.to_string());
			data.erase(0, pos + 2);
			__request.__bodySize += pos + 2;
			if (chunkSize == 0)
				return postDone();
		}
		if (chunkSize < data.length())
		{
			BasicString tmp = data.substr(0, chunkSize);
			__post.processData(tmp);
			data.erase(0, chunkSize);
			__request.__bodySize += chunkSize;
			chunkSize -= data.length();
		}
		else
		{
			BasicString tmp = data;
			__post.processData(tmp);
			size_t len = data.length();
			__request.__bodySize += len;
			chunkSize -= len;
			data.clear();
			break;
		}
	} while (true);
}
void Response::processDefinedBody(BasicString &data)
{
	if (__request.__headers.__contentLength < data.length())
	{
		BasicString tmp = data.substr(0, __request.__headers.__contentLength);
		__request.__headers.__contentLength -= tmp.length();
		__request.__bodySize += tmp.length();
		data.erase(0, tmp.length());
		__post.processData(tmp);
	}
	else
	{
		BasicString tmp = data;
		__request.__bodySize += data.length();
		__request.__headers.__contentLength -= data.length();
		__post.processData(tmp);
		data.clear();
	}
	if (__request.__headers.__contentLength == 0)
		postDone();
}

/*************************************************************************************
 *										  GET										 *
 *************************************************************************************/

void Response::autoindex()
{
	t_svec directories;
	DIR *dir = opendir(__explorer.__fullPath.c_str());
	if (!dir)
		throw ErrorResponse(500, *__location);
	struct dirent *entry;
	while ((entry = readdir(dir)))
	{
		directories.push_back(entry->d_name);
		directories.push_back(" ");
	}
	closedir(dir);
	String body = wsu::buildListingBody(__request.__URI, directories);
	buildResponse(200, body.length());
	__body.join(body);
	__responsePhase = RESPONSE_DONE;
}
bool Response::authenticated()
{
	if (__request.__headers.__cookie.empty())
		return false;
	t_svec cookies = wsu::splitByChar(__request.__headers.__cookie, ';');
	for (t_svec::iterator it = cookies.begin(); it != cookies.end(); it++)
	{
		t_svec cook = wsu::splitByChar(*it, '=');
		if (cook.size() == 2 && !__server->userInDb(cook[1], 0).empty())
			return true;
	}
	return false;
}
void Response::getProcess()
{
	if (__getPhase == GET_INIT)
	{
		if (shouldAuthenticate() && !authenticated())
			throw ErrorResponse(303, __location->__authenticate[1], *__location);
		__get.setWorkers(__explorer, *__location);
		buildResponse(200, wsu::getFileSize(__explorer.__fullPath));
		__getPhase = GET_EXECUTE;
	}
	else
		__get.executeGet(__body);
}

/******************************************************************************
 *									 PHASES 								  *
 ******************************************************************************/

void Response::deletePhase()
{
	if (unlink(__explorer.__fullPath.c_str()) != 0)
		throw ErrorResponse(500, *__location);
	buildResponse(204, 0);
	__responsePhase = RESPONSE_DONE;
}

void Response::cgiPhase()
{
	Cgi cgi(__request);
	cgi.setWorkers(__explorer, *__location);
	String fileName = wsu::generateTimeBasedFileName();
	String filePath = wsu::joinPaths(__location->__serverRoot, fileName);
	cgi.processData(__post.getForm(), filePath);
	this->__tempFiles.push_back(filePath);
	if (__request.__method == GET)
		throw ErrorResponse(307, "/" + fileName, *__location);
	__explorer.__fullPath = filePath;
	__responsePhase = GET_PROCESS;
}

void Response::getPhase()
{
	if (checkCgi())
		__responsePhase = CGI_PROCESS;
	else
	{
		if (__explorer.__type == FILE_)
			getProcess();
		else if (__location->__autoindex)
			autoindex();
		else
			throw ErrorResponse(403, *__location);
	}
}

void Response::postPhase(BasicString &data)
{
	if (__postPhase == POST_INIT)
	{
		__post.setWorkers(__explorer, *__location);
		__postPhase = POST_EXECUTE;
	}
	if (__postPhase == POST_EXECUTE)
	{
		if (__request.__headers.__transferType == DEFINED)
			processDefinedBody(data);
		else if (__request.__headers.__transferType == CHUNKED)
			processCunkedBody(data);
		else
			throw ErrorResponse(400, *__location);
	}
}

void Response::preparePhase()
{
	wsu::debug("preparing response");
	__body.clear();
	__check_methods();
	this->__explorer.prepareRessource(*__location, __request.__URI);
	std::vector<t_method>::iterator it = __location->__allowMethods.begin();
	for (; it != __location->__allowMethods.end() && *it != __request.__method; it++)
		;
	if (it == __location->__allowMethods.end())
		throw ErrorResponse(405, *__location);
}

void Response::processData(BasicString &data)
{
	wsu::info("processing response");
	if (__responsePhase == PREPARING_RESPONSE)
		preparePhase();
	if (__responsePhase == POST_PROCESS)
		postPhase(data);
	if (__responsePhase == GET_PROCESS)
		getPhase();
	if (__responsePhase == CGI_PROCESS)
		cgiPhase();
	if (__responsePhase == DELETE_PROCESS)
		deletePhase();
	if (__responsePhase == RESPONSE_DONE)
		reset();
}
