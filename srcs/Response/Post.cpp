#include "Post.hpp"

Post::Post(Request &request, t_response_phase &phase) : request(request),
														__responsePhase(phase),
														explorer(NULL),
														location(NULL),
														__startTime(std::time(NULL)),
														__phase(MP_INIT)
{
	wsu::debug("Post default constructor");
}

Post::Post(const Post &copy) : request(copy.request),
							   __responsePhase(copy.__responsePhase),
							   explorer(copy.explorer),
							   location(copy.location),
							   __phase(copy.__phase),
							   __data(copy.__data)
{
	wsu::debug("Post copy constructor");
	*this = copy;
}

Post &Post::operator=(const Post &assign)
{
	wsu::debug("Post copy assignement operator");
	if (this != &assign)
	{
		this->__data = assign.__data;
		this->__form = assign.__form;
		this->__phase = assign.__phase;
		this->request = assign.request;
		this->explorer = assign.explorer;
		this->location = assign.location;
		this->__responsePhase = assign.__responsePhase;
	}
	return *this;
}

Post::~Post()
{
	wsu::debug("Post destructor");
	reset();
}

/****************************************************************************
 *								 MINI METHODS								*
 ****************************************************************************/

void Post::reset()
{
	if (__fs.is_open())
		__fs.close();
	__form.clear();
	__data.clear();
	__phase = MP_INIT;
	this->explorer = NULL;
	this->location = NULL;
	request.__headers.__boundary.clear();
}
BasicString &Post::getForm()
{
	return this->__form;
}

/***********************************************************************************************
 *											 METHODS										   *
 ***********************************************************************************************/

void Post::writeDataIntoFile(BasicString &data)
{
	if (!data.empty())
	{
		__fs.write(data.getBuff(), data.length());
		__startTime = std::time(NULL);
	}
}
void Post::createFile(std::vector<String> &headers)
{
	if (__fs.is_open())
		return;
	for (std::vector<String>::iterator it = headers.begin(); it != headers.end(); ++it)
	{
		if (it->find("Content-Disposition:") != String::npos)
		{

			size_t pos = it->find("filename=\"");
			if (pos != String::npos)
			{
				size_t startPos = pos + 10;
				size_t endPos = it->find("\"", startPos);
				String file = it->substr(startPos, endPos - startPos);
				if (file.empty())
					return;
				__fs.open(wsu::joinPaths(explorer->__fullPath, file).c_str());
				if (!__fs.is_open())
					throw wsu::Close();
				return;
			}
			else
				throw wsu::Close();
		}
		else
			throw wsu::Close();
	}
}

void Post::mpBody()
{
	size_t end = __data.find(LINE_BREAK "--" + this->request.__headers.__boundary + "--" LINE_BREAK);
	size_t pos = __data.find(LINE_BREAK "--" + this->request.__headers.__boundary + LINE_BREAK);
	if (pos == String::npos && end == String::npos)
	{
		size_t len = this->request.__headers.__boundary.length() + 8;
		if (__data.length() <= len)
			throw wsu::persist();
		BasicString tmp = __data.substr(0, __data.length() - len);
		__data.erase(0, tmp.length());
		writeDataIntoFile(tmp);
	}
	else if (pos != String::npos)
	{
		size_t len = this->request.__headers.__boundary.length() + 6;
		BasicString tmp = __data.substr(0, pos);
		writeDataIntoFile(tmp);
		__data.erase(0, pos + len);
		__phase = MP_HEADERS;
		__fs.close();
	}
	else if (end != String::npos)
	{
		size_t len = this->request.__headers.__boundary.length() + 8;
		BasicString tmp = __data.substr(0, end);
		writeDataIntoFile(tmp);
		__data.erase(0, end + len);
		__phase = MP_INIT;
	}
}
void Post::mpHeaders()
{
	size_t pos = __data.find(D_LINE_BREAK);
	if (pos == String::npos && __data.length() > REQUEST_MAX_SIZE)
		throw wsu::Close();
	else if (pos == String::npos)
		throw wsu::persist();
	std::vector<String> headers;
	__phase = MP_BODY;
	do
	{
		size_t p = __data.find(LINE_BREAK);
		if (p == 0 || p == String::npos)
			break;
		headers.push_back(__data.substr(0, p).to_string());
		__data.erase(0, p + 2);
	} while (true);
	__data.erase(0, 2);
	createFile(headers);
}
void Post::mpInit()
{
	size_t pos1 = __data.find("--" + request.__headers.__boundary + LINE_BREAK);
	size_t pos2 = __data.find("--" + request.__headers.__boundary + "--" LINE_BREAK);
	if (pos1 == String::npos && pos2 == String::npos && __data.length() < request.__headers.__boundary.length() + 6)
		throw wsu::persist();
	if (pos2 == 0)
	{
		__data.erase(0, request.__headers.__boundary.length() + 6);
		throw wsu::Close();
	}
	if (pos1 != 0)
		throw wsu::Close();
	__data.erase(0, request.__headers.__boundary.length() + 4);
	__phase = MP_HEADERS;
}
void Post::processMultiPartBody()
{
	if (__phase == MP_INIT)
		mpInit();
	if (__phase == MP_HEADERS)
		mpHeaders();
	if (__phase == MP_BODY)
		mpBody();
}
void Post::processFormData()
{
	__form.join(__data);
	if (__form.length() > FORM_MAX_SIZE)
		throw wsu::Close();
}
void Post::setWorkers(FileExplorer &explorer, Location &location)
{
	this->location = &location;
	this->explorer = &explorer;
	__startTime = std::time(NULL);
}
void Post::processData(BasicString &data)
{
	if (!explorer || !location)
		return wsu::fatal("no objects to be referenced");

	__data.join(data);
	if (request.__bodySize > location->__clientBodyBufferSize)
		throw wsu::Close();
	if (std::time(NULL) - __startTime > CLIENT_TIMEOUT)
		throw wsu::Close();

	if (request.__headers.__contentType == FORM)
		processFormData();
	else if (request.__headers.__contentType == MULTIPART)
		processMultiPartBody();
	else
		throw wsu::Close();
}
