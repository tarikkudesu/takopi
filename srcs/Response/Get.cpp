#include "Get.hpp"

Get::Get(Request &request, t_response_phase &phase) : request(request),
													  __responsePhase(phase),
													  explorer(NULL),
													  location(NULL),
													  __phase(OPEN_FILE)
{
	wsu::debug("Get default constructor");
}
Get::Get(const Get &copy) : request(copy.request),
							__responsePhase(copy.__responsePhase),
							explorer(copy.explorer),
							location(copy.location),
							__phase(copy.__phase)
{
	wsu::debug("Get copy constructor");
	*this = copy;
}
Get &Get::operator=(const Get &assign)
{
	wsu::debug("Get copy assignement operator");
	if (this != &assign)
	{
		this->__phase = assign.__phase;
		this->request = assign.request;
		this->explorer = assign.explorer;
		this->location = assign.location;
		this->__responsePhase = assign.__responsePhase;
	}
	return *this;
}
Get::~Get()
{
	wsu::debug("Get destructor");
	reset();
}

void Get::reset()
{
	wsu::debug("closing file");
	if (__file.is_open())
		__file.close();
	this->__bodySize = 0;
	this->explorer = NULL;
	this->location = NULL;
	__phase = OPEN_FILE;
	__responsePhase = RESPONSE_DONE;
}

/***********************************************************************************************
 *											 METHODS										   *
 ***********************************************************************************************/

void Get::getInPhase()
{
	wsu::debug("init file reading");
	this->__bodySize = wsu::getFileSize(explorer->__fullPath);
	this->__file.open(explorer->__fullPath.c_str(), std::ios::binary);
	if (!__file.is_open())
		throw ErrorResponse(403, *location);
	__phase = READ_FILE;
}
void Get::duringGetPhase(BasicString &body)
{
	wsu::debug("reading file");
	char buffer[READ_SIZE];
	wsu::bzero(buffer, READ_SIZE);
	if (!__file.good())
		wsu::fatal("bad stream");
	__file.read(buffer, sizeof(buffer));
	body = BasicString(buffer, __file.gcount());
	if (this->__bodySize == 0)
		__phase = CLOSE_FILE;
	this->__bodySize -= __file.gcount();
}
void Get::setWorkers(FileExplorer &explorer, Location &location)
{
	this->location = &location;
	this->explorer = &explorer;
}
void Get::executeGet(BasicString &body)
{
	if (!explorer || !location)
		return wsu::fatal("no objects to be referenced");
	if (__phase == OPEN_FILE)
		getInPhase();
	if (__phase == READ_FILE)
		duringGetPhase(body);
	if (__phase == CLOSE_FILE)
		reset();
}
