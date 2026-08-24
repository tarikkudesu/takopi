#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include "../utilities/BasicString.hpp"

class Client
{
	private:
		Client(const Client &copy);
		Client &operator=(const Client &assign);

	public:
		void							handleMessage(const String &message);

		Client();
		~Client();
};

#endif
