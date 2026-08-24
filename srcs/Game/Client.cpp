#include "Client.hpp"

Client::Client()
{
}
Client::Client(const Client &copy)
{
	*this = copy;
}
Client &Client::operator=(const Client &assign)
{
	if (this != &assign)
	{
	}
	return *this;
}
Client::~Client()
{
}
