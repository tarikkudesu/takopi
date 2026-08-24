#ifndef __ERRORRESPONSE_HPP__
# define __ERRORRESPONSE_HPP__

# include "../ServerManager/Server.hpp"
class ErrorResponse
{
	private :

		static String						__errPage;
        String								__page;
		int16_t								__code;
		String								__Body;
		String								__cookie;
		String								__headers;
		Location							*__location;
		String 								__redirection;
		String								__reasonPhrase;

		void								buildStatusLine();
		void								buildHeaderFeilds();
		void								buildResponseBody();
		void								constructErrorPage();
		ErrorResponse();

	public:
		String								__StatusLine;

		BasicString							getResponse() const;
		void								print() const;

		ErrorResponse( int code );
		ErrorResponse( int code, Location &location );
		ErrorResponse( int code, String redirection, Location &location);
		ErrorResponse( int code, String redirection, String cookie, Location &location);
		ErrorResponse( const ErrorResponse &copy );
		ErrorResponse	&operator=( const ErrorResponse &assign );
		~ErrorResponse();
};

#endif
