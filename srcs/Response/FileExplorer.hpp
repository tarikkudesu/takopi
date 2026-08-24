#pragma once

# include "../Request/Request.hpp"

class FileExplorer
{
	private:
		void			loadType(const char* path);

	public:
		t_type			__type;
		String			__fullPath;
		Location		*__location;

        String          setAlias(String uri);
		void			changeRequestedFile(String file);
		void			loadPathExploring(const String& uri);
		void			prepareRessource(Location& location, const String& uri);

		FileExplorer();
		FileExplorer(const FileExplorer &copy);
		FileExplorer &operator=(const FileExplorer &assign);
		~FileExplorer();
};

