#ifndef __ENUMS_HPP__
#define __ENUMS_HPP__

# include "../utilities/BasicString.hpp"

# define INITIAL_FOOD								10
# define FOOD_LIFE_UNITS							126
# define CLIENTS_PER_TEAM							6
# define WIN_PLAYER_COUNT							6
# define MAX_PENDING_COMMANDS						10
# define EGG_HATCH_DURATION							600
# define MAP_MAX_SIZE								100
# define WIN_LEVEL									8

typedef enum e_resource
{
	NOURRITURE = 0,
	LINEMATE,
	DERAUMERE,
	SIBUR,
	MENDIANE,
	PHIRAS,
	THYSTAME,
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

typedef enum e_type
{
	GAME = 0,
	ADMIN,
	GUI,
	UNKNOWN
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
	CMD_ADMIN_GAMES,
	CMD_UNKNOWN,
} t_command;

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
} t_elevation_req;

typedef struct s_notification
{
	int playerId;
	String message;
} t_notification;

#endif
