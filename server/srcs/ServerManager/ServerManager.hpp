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
		void						overrideargs(const t_args &overrides);
		String						checkOuterscope(String outerScope);

		ServerManager() = delete;
		ServerManager( const ServerManager &copy ) = delete;
		ServerManager	&operator=( const ServerManager &assign ) = delete;

	public:
		bool						runzappy(const t_args &overrides);

		ServerManager( const String &configutation_file );
		~ServerManager();
};

#endif
