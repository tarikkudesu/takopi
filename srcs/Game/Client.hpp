#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

class Client
{
	private:
	public:
		Client();
		Client(const Client &copy);
		Client &operator=(const Client &assign);
		~Client();
};

#endif