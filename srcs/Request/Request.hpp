#ifndef __REQUEST_HPP__
# define __REQUEST_HPP__

# include "Headers.hpp"

class Request
{
	private:
		t_connection_phase				&__connectionPhase;
		BasicString 					__data;
		std::time_t						__startTime;
		String							__requestLine;
		String							__requestHeaders;

		void							validateURI();
		void							requestInit();
		void							parseRequest();
		void							validateHeaders();
		void							validateRequestLine();
		void							requestExecute(BasicString &data);

	public:
		String							__URI;
		t_method						__method;
		Headers							__headers;
		size_t							__bodySize;
		String							__fragement;
		String							__protocole;
		String							__queryString;
		std::map< String, String >		__headerFeilds;
		t_request_phase 				__requestPhase;

		void							reset();
		void							processData(BasicString &data);

		Request(t_connection_phase &phase);
		Request( const Request &copy );
		Request& operator=( const Request &assign );
		~Request();
};

#endif
