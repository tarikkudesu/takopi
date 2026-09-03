#ifndef __SERVERMANAGER_HPP__
# define __SERVERMANAGER_HPP__

# include "Core.hpp"

/*************************************************************************
 *                             SERVER PARSER                             *
 *************************************************************************/

class ServerManager
{
	private :
		String						__lines;
		String						__config;
		t_serVect					__serverTemplates;

		void						readFile();
		void						firstCheck();
		void						checkBraces();
		void						initServers();
		void						reduceSpaces();
		void						setUpServers();
		void						setUpServer(size_t start);
		String						checkOuterscope(String outerScope);
		ServerManager();

	public:
		void						setUpZappy();

		ServerManager( const ServerManager &copy );
		ServerManager( const String &configutation_file );
		ServerManager	&operator=( const ServerManager &assign );
		~ServerManager();
};

#endif
