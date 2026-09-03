#ifndef __ENUMS_HPP__
#define __ENUMS_HPP__

#include "../utilities/BasicString.hpp"

/*************************************************************************
 *                           RESOURCE TYPES                              *
 *************************************************************************/

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

/*************************************************************************
 *                          PLAYER DIRECTION                             *
 *************************************************************************/

typedef enum e_direction
{
	NORTH = 0,
	EAST,
	SOUTH,
	WEST
} t_direction;

/*************************************************************************
 *                         PLAYER LIFECYCLE                              *
 *************************************************************************/

typedef enum e_player_state
{
	PLAYER_HANDSHAKE = 0,
	PLAYER_ALIVE,
	PLAYER_DEAD
} t_player_state;

/*************************************************************************
 *                            CONNECTION                                 *
 *************************************************************************/

typedef enum e_type
{
	GAME = 0,
	ADMIN,
	GUI,
	UNKNOWN
} t_type;

enum e_tls_operation
{
	TLS_OPERATION_NONE,
	TLS_OPERATION_READ,
	TLS_OPERATION_WRITE,
	TLS_OPERATION_HANDSHAKE
};

enum e_tls_wait
{
	TLS_WAIT_READ,
	TLS_WAIT_WRITE
};

/*************************************************************************
 *                          COMMAND TYPES                                *
 *************************************************************************/

typedef enum e_command
{
	CMD_AVANCE = 0,
	CMD_DROITE,
	CMD_GAUCHE,
	CMD_VOIR,
	CMD_INVENTAIRE,
	CMD_PREND,
	CMD_POSE,
	CMD_EXPULSE,
	CMD_BROADCAST,
	CMD_INCANTATION,
	CMD_FORK,
	CMD_CONNECT_NBR,
	CMD_UNKNOWN
} t_command;

/*************************************************************************
 *                        DATA STRUCTURES                                *
 *************************************************************************/

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

/*************************************************************************
 *                          GAME CONSTANTS                               *
 *************************************************************************/

#define FOOD_LIFE_UNITS 126
#define INITIAL_FOOD 10
#define MAX_PENDING_COMMANDS 10
#define EGG_HATCH_DURATION 600
#define WIN_LEVEL 8
#define WIN_PLAYER_COUNT 6

#endif
