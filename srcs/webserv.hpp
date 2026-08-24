#ifndef __WEBSERV_HPP__
#define __WEBSERV_HPP__

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
#define CLIENT_TIMEOUT 15
#define CONNECTION_TIMEOUT 15

#define MAX_EVENTS 1024

#define READ_SIZE 131072
#define MAX_MESSAGE_SIZE 8192

#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_PORT 4242
#define DEFAULT_CONFIG_FILE_PATH "./zappy.conf"

#define NEWLINE "\n"

#define USAGE                                                                                      \
	"\nUsage: ./zappy [OPTIONS]\n\nOptions:\n"                                                     \
	"\t-p, --port <number>\t\tport the server listens on\n"                                        \
	"\t-x, --width <number>\t\tmap width\n"                                                        \
	"\t-y, --height <number>\tmap height\n"                                                        \
	"\t-n, --names <n1> <n2>...\tteams names\n"                                                    \
	"\t-c, --config <file>\t\tconfiguration file fallback (default: " DEFAULT_CONFIG_FILE_PATH ")\n" \
	"\t-l, --logs string\t\tlog events(\"debug\"|\"info\"|\"warn\"|\"error\"|\"fatal\"|\"all\")\n"

typedef enum e_endian
{
	SERVER,
	CONNECTION,
} t_endian;

#endif
