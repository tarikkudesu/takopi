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

#define RED								"\033[1;31m"
#define BLUE							"\033[1;34m"
#define CYAN							"\033[1;36m"
#define RESET							"\033[1;0m"
#define GREEN							"\033[1;32m"
#define YELLOW							"\033[1;33m"
#define MAGENTA							"\033[1;35m"

#define SELECT_TIMEOUT					1000
#define MAX_EVENTS						1024
#define READ_SIZE						1024
#define MAX_MESSAGE_SIZE				1024
#define MAX_ADMIN_AUTH_FAILURES			3

#define DEFAULT_HOST					"0.0.0.0"
#define DEFAULT_CERTIFICATE				"certs/server.crt"
#define DEFAULT_PRIVATE_KEY				"certs/server.key"
#define PLAYER_DEATH_MESSAGE			"mort" NEWLINE

#define NEWLINE							"\n"
#define KO								"ko" NEWLINE
#define OK								"ok" NEWLINE
#define ADMIN_OK						GREEN  "[OK]"	RESET " "
#define ADMIN_ERR						RED    "[ERR]"	RESET " "
#define ADMIN_INFO						BLUE   "[INFO]" RESET " "
#define ADMIN_WARN						YELLOW "[WARN]" RESET " "
#define ADMIN_PROMPT					GREEN "➜ " CYAN "zappy " RED "admin" YELLOW " ✗ " RESET
#define PRINTABLE						" \t\n\r\v\f0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define WHITESPACE						" \t\n\r\v\f"

#define INITIAL_FOOD					10
#define FOOD_LIFE_UNITS					99999
#define MAX_PENDING_COMMANDS			10
#define EGG_HATCH_DURATION				600
#define MAP_MAX_SIZE					100
#define WIN_PLAYERS						6
#define WIN_LEVEL						8


typedef enum e_resource
{
	NOURRITURE = 0,
	LINEMATE = 1,
	DERAUMERE = 2,
	SIBUR = 3,
	MENDIANE = 4,
	PHIRAS = 5,
	THYSTAME = 6,
	RESOURCE_COUNT = 7
} Resource;

typedef enum e_direction
{
	NORTH = 0,
	EAST,
	SOUTH,
	WEST
} Direction;

typedef enum e_protocol_verb
{
	PROTOCOL_WELCOME,
	PROTOCOL_GRAPHIC,
	PROTOCOL_MSZ,
	PROTOCOL_BCT,
	PROTOCOL_MCT,
	PROTOCOL_TNA,
	PROTOCOL_PNW,
	PROTOCOL_PPO,
	PROTOCOL_PLV,
	PROTOCOL_PIN,
	PROTOCOL_PEX,
	PROTOCOL_PBC,
	PROTOCOL_PIC,
	PROTOCOL_PIE,
	PROTOCOL_PFK,
	PROTOCOL_PDR,
	PROTOCOL_PGT,
	PROTOCOL_PDI,
	PROTOCOL_ENW,
	PROTOCOL_EHT,
	PROTOCOL_EBO,
	PROTOCOL_EDI,
	PROTOCOL_SGT,
	PROTOCOL_SST,
	PROTOCOL_SEG,
	PROTOCOL_SMG,
	PROTOCOL_SUC,
	PROTOCOL_SBP,
	PROTOCOL_UNKNOWN
} ProtocoleVerb;

typedef enum e_player_state
{
	PLAYER_HANDSHAKE = 0,
	PLAYER_ALIVE,
	PLAYER_DEAD
} PlayerState;

typedef enum e_game_state
{
	GAME_RUNNING,
	GAME_ENDING
} GameState;

typedef enum e_gui_connection_state
{
	GUI_WAITING_HANDSHAKE,
	GUI_READY,
	GUI_CLOSING
} GuiConnectionState;

typedef enum e_admin_connection_state
{
	ADMIN_TLS_HANDSHAKE,
	ADMIN_AUTHENTICATING,
	ADMIN_READY,
	ADMIN_CLOSING,
	ADMIN_FAILED
} AdminConnectionState;

typedef enum e_egg_state
{
	EGG_INCUBATING,
	EGG_HATCHED
} EggState;

typedef enum e_type
{
	GAME = 0,
	GUI,
	ADMIN
} Type;

typedef enum e_tls_operation
{
	TLS_OPERATION_NONE,
	TLS_OPERATION_READ,
	TLS_OPERATION_WRITE,
	TLS_OPERATION_HANDSHAKE
} TLSOperation;

typedef enum e_tls_wait
{
	TLS_WAIT_READ,
	TLS_WAIT_WRITE
} TLSWait;

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
} CommandType;

typedef struct s_elevation_req
{
    int level;
    int players_needed;
    int linemate;
    int deraumere;
    int sibur;
    int mendiane;
    int phiras;
    int thystame;
} ElevationReq;

typedef struct s_notification
{
	int playerId;
	String message;
} Notification;

typedef struct s_gui_event
{
	unsigned long sequence;
	String payload;
} GuiEvent;

typedef struct s_incantation_context
{
	int x;
	int y;
	int level;
	std::vector<int> playerIds;
} IncantationContext;

#endif
