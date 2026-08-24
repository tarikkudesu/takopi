#ifndef __CGI_HPP__
#define __CGI_HPP__

# include "Get.hpp"

class Cgi
{
	private:
		Request 					&__request;

		FileExplorer				*__explorer;
		Location					*__location;

		int 						__fd;
		char						**env;

		void						reset();
		void						execute(const char* path);
		String						getPathInfo(BasicString &reqBody);
		String						getQueryString( BasicString &reqBody );
		void						setCgiEnvironement( BasicString &reqBody );


	public:
		void						setWorkers(FileExplorer &explorer, Location &location);
		void						processData( BasicString &reqBody, String file );

		Cgi(Request &request);
		Cgi(const Cgi& cgi);
		Cgi	&operator=(const Cgi& cgi);
		~Cgi();
};

#endif