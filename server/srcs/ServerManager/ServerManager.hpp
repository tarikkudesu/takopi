#ifndef __SERVERMANAGER_HPP__
# define __SERVERMANAGER_HPP__

# include "Core.hpp"

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
		void						validateServerCardinality();
		String						checkOuterscope(String outerScope);

		ServerManager() = delete;
		ServerManager( const ServerManager &copy ) = delete;
		ServerManager	&operator=( const ServerManager &assign ) = delete;

	public:
		bool						setUpZappy();

		ServerManager( const String &configutation_file );
		~ServerManager();
};

#endif
