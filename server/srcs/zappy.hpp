#ifndef __ZAPPY_HPP__
#define __ZAPPY_HPP__

#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <netdb.h>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <limits.h>
#include <errno.h>

typedef std::string String;
typedef std::vector<String> t_svec;
typedef std::map<String, String> Map;

#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define CYAN "\033[1;36m"
#define RESET "\033[1;0m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define MAGENTA "\033[1;35m"

#define SELECT_TIMEOUT 5000

#define MAX_EVENTS 1024

#define READ_SIZE 1024
#define MAX_MESSAGE_SIZE 1024

#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_CERTIFICATE "certs/server.crt"
#define DEFAULT_PRIVATE_KEY "certs/server.key"

#define NEWLINE "\n"

#define PRINTABLE " \t\n\r\v\f0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"

typedef enum e_endian
{
	SERVER,
	CONNECTION,
} t_endian;

#endif
