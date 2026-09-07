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

#define RED 							"\033[1;31m"
#define BLUE 							"\033[1;34m"
#define CYAN 							"\033[1;36m"
#define RESET 							"\033[1;0m"
#define GREEN 							"\033[1;32m"
#define YELLOW 							"\033[1;33m"
#define MAGENTA 						"\033[1;35m"

#define SELECT_TIMEOUT 					5000
#define MAX_EVENTS 						1024
#define READ_SIZE 						1024
#define MAX_MESSAGE_SIZE 				1024
#define MAX_ADMIN_AUTH_FAILURES 		3

#define DEFAULT_HOST 					"0.0.0.0"
#define DEFAULT_CERTIFICATE 			"certs/server.crt"
#define DEFAULT_PRIVATE_KEY 			"certs/server.key"
#define PLAYER_DEATH_MESSAGE 			"mort" NEWLINE

#define NEWLINE 						"\n"
#define ADMIN_OK    					GREEN  "[OK]"   RESET " "
#define ADMIN_ERR   					RED    "[ERR]"  RESET " "
#define ADMIN_INFO  					BLUE   "[INFO]" RESET " "
#define ADMIN_WARN  					YELLOW "[WARN]" RESET " "
#define ADMIN_PROMPT 					GREEN "➜ " CYAN "zappy " RED "admin" YELLOW " ✗ " RESET
#define PRINTABLE 						" \t\n\r\v\f0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"

#define INITIAL_FOOD								10
#define FOOD_LIFE_UNITS								126
#define CLIENTS_PER_TEAM							6
#define MAX_PENDING_COMMANDS						10
#define EGG_HATCH_DURATION							600
#define MAP_MAX_SIZE								100
#define WIN_LEVEL									8


typedef enum e_resource
{
	NOURRITURE = 0,
	DERAUMERE,
	LINEMATE,
	SIBUR,
	PHIRAS,
	THYSTAME,
	MENDIANE,
	RESOURCE_COUNT
} t_resource;

typedef enum e_direction
{
	NORTH = 0,
	EAST,
	SOUTH,
	WEST
} t_direction;

typedef enum e_player_state
{
	PLAYER_HANDSHAKE = 0,
	PLAYER_ALIVE,
	PLAYER_DEAD
} t_player_state;

typedef enum e_game_state
{
	GAME_RUNNING,
	GAME_ENDING
} t_game_state;

typedef enum e_type
{
	GAME = 0,
	GUI,
	ADMIN
} t_type;

typedef enum e_tls_operation
{
	TLS_OPERATION_NONE,
	TLS_OPERATION_READ,
	TLS_OPERATION_WRITE,
	TLS_OPERATION_HANDSHAKE
} t_tls_operation;

typedef enum e_tls_wait
{
	TLS_WAIT_READ,
	TLS_WAIT_WRITE
} t_tls_wait;

typedef enum e_command
{
	CMD_AVANCE = 0,
	CMD_VOIR,
	CMD_FORK,
	CMD_POSE,
	CMD_PREND,
	CMD_DROITE,
	CMD_GAUCHE,
	CMD_EXPULSE,
	CMD_BROADCAST,
	CMD_INVENTAIRE,
	CMD_INCANTATION,
	CMD_CONNECT_NBR,
	CMD_ADMIN_HELP,
	CMD_ADMIN_RESIZE,
	CMD_ADMIN_RETIME,
	CMD_UNKNOWN,
} t_command;

typedef struct s_elevation_req
{
	int level;
	int players_needed;
	int deraumere;
	int linemate;
	int mendiane;
	int thystame;
	int phiras;
	int sibur;
} t_elevation_req;

typedef struct s_notification
{
	int playerId;
	String message;
} t_notification;

#endif
