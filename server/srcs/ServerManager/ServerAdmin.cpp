#include "ServerAdmin.hpp"

ServerAdmin::ServerAdmin(String line) : Server(ADMIN),
										__ctx(NULL),
										__certificate(DEFAULT_CERTIFICATE),
										__privateKey(DEFAULT_PRIVATE_KEY),
										__password("")
{
	mzu::debug("ServerAdmin constructor");
	parseBlock(line);
	if (!this->__portSet)
		throw std::runtime_error("admin server: missing \"port\" directive");
	if (this->__password.empty())
		throw std::runtime_error("admin server: missing \"password\" directive");
}
ServerAdmin::~ServerAdmin()
{
	SSL_CTX_free(__ctx);
	mzu::debug("ServerAdmin destructor");
}

void	ServerAdmin::proccessCertificateToken( t_svec &tokens )
{
	if (this->__certificate != DEFAULT_CERTIFICATE)
		throw std::runtime_error(tokens.at(0) + " directive is duplicate");
	if (tokens.size() == 1)
		throw std::runtime_error(tokens.at(0) + ": no private key value");
	if (tokens.size() > 2)
		throw std::runtime_error(tokens.at(0) + ": multiple private key values");
	this->__certificate = tokens.at(1);
}

void	ServerAdmin::proccessPrivateKeyToken( t_svec &tokens )
{
	if (this->__privateKey != DEFAULT_PRIVATE_KEY)
		throw std::runtime_error(tokens.at(0) + " directive is duplicate");
	if (tokens.size() == 1)
		throw std::runtime_error(tokens.at(0) + ": no private key value");
	if (tokens.size() > 2)
		throw std::runtime_error(tokens.at(0) + ": multiple private key values");
	this->__privateKey = tokens.at(1);
}

void	ServerAdmin::proccessPasswordToken( t_svec &tokens )
{
	if (!this->__password.empty())
		throw std::runtime_error(tokens.at(0) + " directive is duplicate");
	if (tokens.size() == 1)
		throw std::runtime_error(tokens.at(0) + ": no password value");
	if (tokens.size() > 2)
		throw std::runtime_error(tokens.at(0) + ": multiple password values");
	this->__password = tokens.at(1);
}

void ServerAdmin::proccessToken(t_svec &tokens)
{
	String key = tokens.at(0);
	if (key != "port" && key != "host" && key != "certificate" && key != "private_key" && key != "password")
		throw std::runtime_error(key + ": unknown directive");
	if (key == "port")
		proccessPortToken(tokens);
	else if (key == "host")
		proccessHostToken(tokens);
	else if (key == "certificate")
		proccessCertificateToken(tokens);
	else if (key == "private_key")
		proccessPrivateKeyToken(tokens);
	else if (key == "password")
		proccessPasswordToken(tokens);
}

bool ServerAdmin::checkPassword(const String &password) const
{
	if (password.length() != this->__password.length())
		return false;
	return CRYPTO_memcmp(password.data(), this->__password.data(), password.length()) == 0;
}

void ServerAdmin::furtherSetup()
{
	if (this->__ctx)
	{
		SSL_CTX_free(this->__ctx);
		this->__ctx = NULL;
	}
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	const SSL_METHOD* method = TLS_server_method();
	SSL_CTX *ctx = SSL_CTX_new(method);
	if (!ctx)
		throw std::runtime_error("admin server: could not create a new SSL context");

	SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

	if (SSL_CTX_use_certificate_file(ctx, this->__certificate.c_str(), SSL_FILETYPE_PEM) <= 0)
	{
		SSL_CTX_free(ctx);
		throw std::runtime_error("admin server: Could not use SSL certificate file");
	}
	if (SSL_CTX_use_PrivateKey_file(ctx, this->__privateKey.c_str(), SSL_FILETYPE_PEM) <= 0)
	{
		SSL_CTX_free(ctx);
		throw std::runtime_error("admin server: Could not use SSL private key file");
	}
	if (!SSL_CTX_check_private_key(ctx))
	{
		SSL_CTX_free(ctx);
		throw std::runtime_error("admin server: Private key does not match certificate");
	}
	this->__ctx = ctx;
}

SSL_CTX	*ServerAdmin::getTLSContext()
{
	return this->__ctx;
}
