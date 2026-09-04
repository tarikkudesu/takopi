#ifndef __ENUMS_HPP__
#define __ENUMS_HPP__

#include "zappy.hpp"

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

typedef struct s_tile_content
{
	int players;
	int resources[RESOURCE_COUNT];
} t_tile_content;

#endif
