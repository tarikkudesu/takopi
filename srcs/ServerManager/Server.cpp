#include "Server.hpp"

Server::Server(int port, const String &host) : __sd(-1),
											   __port(port),
											   __host(host.empty() ? DEFAULT_HOST : host)
{
}

Server::~Server()
{
	if (__sd != -1)
		close(__sd);
}

int Server::getServerSocket() const
{
	return this->__sd;
}
int Server::getServerPort() const
{
	return this->__port;
}
void Server::setPort(int port)
{
	this->__port = port;
}
const String &Server::getServerHost() const
{
	return this->__host;
}
String Server::serverIdentity() const
{
	return this->__host + ":" + wsu::intToString(this->__port);
}

void Server::setup()
{
	struct sockaddr_in addr;
	int ra = 1, rp = 1;

	this->__sd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->__sd == -1)
		throw std::runtime_error(serverIdentity() + ": non functional: failed to create socket ");
	try
	{
		if (-1 == setsockopt(__sd, SOL_SOCKET, SO_REUSEADDR, (void *)&ra, sizeof(ra)))
			throw std::runtime_error(serverIdentity() + ": non functional: failed to make reusable address");
		if (-1 == setsockopt(__sd, SOL_SOCKET, SO_REUSEPORT, (void *)&rp, sizeof(rp)))
			throw std::runtime_error(serverIdentity() + ": non functional: failed to make reusable port");
		addr.sin_family = AF_INET;
		{
			struct addrinfo hint;
			struct addrinfo *result;
			wsu::bzero(&hint, sizeof(hint));
			hint.ai_family = AF_INET;
			hint.ai_socktype = SOCK_STREAM;
			int status = getaddrinfo(this->__host.c_str(), NULL, &hint, &result);
			if (status == 0)
			{
				struct sockaddr_in *add = (struct sockaddr_in *)result->ai_addr;
				addr.sin_addr.s_addr = add->sin_addr.s_addr;
				freeaddrinfo(result);
			}
			else
			{
				freeaddrinfo(result);
				throw std::runtime_error(serverIdentity() + " non functional: couldn't resolve server host name: " + this->__host);
			}
		}
		addr.sin_port = htons(this->__port);
		if (-1 == bind(this->__sd, (struct sockaddr *)&addr, sizeof(addr)))
			throw std::runtime_error(serverIdentity() + " non functional: failed to bind socket");
		if (-1 == listen(this->__sd, SOMAXCONN))
			throw std::runtime_error(serverIdentity() + " non functional: failed to listen for connections");
	}
	catch (std::runtime_error &e)
	{
		close(this->__sd);
		this->__sd = -1;
		throw e;
	}
}
