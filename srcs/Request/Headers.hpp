#ifndef __HEADERS_HPP__
# define __HEADERS_HPP__

# include "../Response/ErrorResponse.hpp"

class Headers
{
	private:
		void						range( std::map< String, String > &headers );
		void						cookie( std::map< String, String > &headers );
		void						contentType( std::map< String, String > &headers );
		void						hostAndPort( std::map< String, String > &headers );
		void						contentLength( std::map< String, String > &headers );
		void						connectionType( std::map< String, String > &headers );
		void						transferEncoding( std::map< String, String > &headers );
		String						getHeaderFeildValue(const String &key, std::map< String, String > &headers);


	public:
		int							__port;
		String						__host;
		String						__cookie;
		String						__boundary;
		t_content_type				__contentType;
		size_t						__contentLength;
		String						__transferEncoding;
		t_connectionType			__connectionType;
		t_bodyType					__transferType;

		void						clear();
		void						parseHeaders(std::map< String, String > &headers);

		Headers();
		Headers( const Headers &copy );
		Headers& operator=( const Headers &assign );
		~Headers();
};

std::ostream &operator<<(std::ostream &o, const Headers &h );

#endif
