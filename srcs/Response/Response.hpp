#ifndef __RESPONSE_HPP__
#define __RESPONSE_HPP__

#include "Cgi.hpp"

class Response
{
	private:
		t_connection_phase				&__connectionPhase;
		t_response_phase				__responsePhase;
		t_post_phase					__postPhase;
		t_get_phase						__getPhase;
		Get								__get;
		Post							__post;
		BasicString 					__body;
		FileExplorer					__explorer;
		Server							*__server;
		Request							&__request;
		Location						*__location;
		t_svec							__tempFiles;


		void							cgiPhase();
		void							postDone();
		bool							checkCgi();
		void							getPhase();
		void							autoindex();
		void							getProcess();
		void							deletePhase();
		void							preparePhase();
        bool                            authenticated();
		void							__check_methods();
		bool							shouldAuthenticate();
		void							postPhase(BasicString &data);
		void						    processCunkedBody(BasicString &data);
		void						    processDefinedBody(BasicString &data);
		void							buildResponse(int code, ssize_t length);

	public:

		void							reset();
		BasicString 					getResponse();
		void							setupWorkers(Server &server, Location &location);
		void							processData(BasicString &data);


		Response(t_connection_phase &phase, Request &request);
		Response(const Response &copy);
		Response &operator=(const Response &assign);
		~Response();
};

#endif
