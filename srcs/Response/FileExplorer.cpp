#include "./FileExplorer.hpp"

FileExplorer::FileExplorer()
{
	wsu::debug("FileExplorer default constructor");
}

FileExplorer::FileExplorer(const FileExplorer &copy) : __type(copy.__type),
																   __fullPath(copy.__fullPath),
																   __location(copy.__location)
{
	wsu::debug("FileExplorer copy constructor");
	*this = copy;
}
FileExplorer &FileExplorer::operator=(const FileExplorer &assign)
{
	wsu::debug("FileExplorer copy assignement operator");
	if (this != &assign)
	{
		__type = assign.__type;
		__location = assign.__location;
		__fullPath = assign.__fullPath;
	}
	return *this;
}
FileExplorer::~FileExplorer()
{
	wsu::debug("FileExplorer destructor");
}
void FileExplorer::loadType(const char *path)
{
	struct stat file_stat;
	if (stat(path, &file_stat) == -1)
		throw ErrorResponse(404, *__location);
	else if (!(file_stat.st_mode & S_IRUSR))
		throw ErrorResponse(401, *__location);
	if (S_ISDIR(file_stat.st_mode))
		__type = FOLDER;
	else
		__type = FILE_;
}
void	FileExplorer::changeRequestedFile(String file)
{
	__fullPath = wsu::joinPaths(__location->__root, file);
}
String FileExplorer::setAlias(String uri)
{
	if (__location->__alias.empty())
		return uri;
	size_t endPos = uri.find(__location->__path);
	if (endPos == String::npos)
		return uri;
	String newStr = uri.substr(endPos + __location->__path.size());
	newStr = __location->__alias + newStr;
	return newStr;
}
void FileExplorer::loadPathExploring(const String& uri)
{	
	String newUri = setAlias(uri);
	__fullPath = wsu::joinPaths(__location->__root, newUri);
	loadType(__fullPath.c_str());
	if (__type == FOLDER)
	{
		t_svec::iterator it = __location->__index.begin();
		for (; it != __location->__index.end(); ++it)
		{
			String s = wsu::joinPaths(__fullPath, *it);
			if (!access(s.c_str(), F_OK))
			{
				__fullPath = s;
				__type = FILE_;
				return;
			}
		}
	}
}
void FileExplorer::prepareRessource(Location& location, const String& uri)
{
	__fullPath.clear();
	__location = &location;
	if (wsu::__criticalOverLoad == true)
		throw ErrorResponse(503, *__location);
	if (!__location->__return.empty())
		throw ErrorResponse(301, __location->__return, *__location);
	loadPathExploring(uri);
}

