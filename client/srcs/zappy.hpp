#ifndef __ZAPPY_HPP__
#define __ZAPPY_HPP__

#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

typedef std::string String;
typedef std::vector<String> t_svec;

#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define CYAN "\033[1;36m"
#define RESET "\033[1;0m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define MAGENTA "\033[1;35m"

#define SELECT_TIMEOUT 5000

#define READ_SIZE 1024
#define MAX_MESSAGE_SIZE 1024

#define DEFAULT_HOST "localhost"

#define NEWLINE "\n"

#define FOOD_SAFETY_THRESHOLD 3
#define INVENTORY_POLL_INTERVAL 10
#define WIN_LEVEL 8

#endif
