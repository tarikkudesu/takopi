#ifndef __LOCATION_HPP__
# define __LOCATION_HPP__

# include "../utilities/BasicString.hpp"

class Location
{
	private :
		bool								b__r; // temporary usage

		void								proccessClientBodyBufferSizeToken( t_svec &tokens );
		void								proccessAllowMethodsDirective( t_svec &tokens );
		void								proccessAuthenticateDirective(t_svec &tokens);
		void								proccessErrorPageDirective( t_svec &tokens );
		void								proccessAutoindexDirective( t_svec &tokens );
		void								proccessCgiPassDirective( t_svec &tokens );
		void								proccessReturnDirective( t_svec &tokens );
		void								proccessAliasDirective( t_svec &tokens );
		void								proccessIndexDirective( t_svec &tokens );
		void								proccessRootDirective( t_svec &tokens );
		void								proccessToken( t_svec &tokens );
		void								LocationBlock( size_t pos );
		void								addDirective( size_t end );
		void								proccessDirectives();
		void								parseDirectives();
		void								parse();
		Location();

	public:
		String      						__path;
		String								__line; // temporary usage
		String								__root;
		String								__alias;
		t_svec								__index;
		String								__return;
		String								__cgiPass;
		bool								__autoindex;
		String								__serverRoot;
		std::deque< String >				__directives;
		std::map< int16_t, String >			__errorPages;
		std::vector< t_method >				__allowMethods;
		std::vector< String >				__authenticate;
		size_t								__clientBodyBufferSize;


		Location( const Location &copy );
		Location( const String &conf, const String &root );
		Location( const String &dir, const String &conf, const String &root );
		Location	&operator=( const Location &assign );
		~Location();
};

#endif
