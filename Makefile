# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: tamehri <tamehri@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/09/03 23:50:00 by ooulcaid          #+#    #+#              #
#    Updated: 2026/10/11 13:11:00 by ooulcaid         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# ==============================================================================
#                               COMPILERS & FLAGS
# ==============================================================================
CXX				= g++
CXXFLAGS		= -Wall -Wextra -Werror -std=c++11 -g -O0 -MMD -MP
RAYLIB_DIR		?= /usr/local
RAYLIB_INCLUDE	?= $(RAYLIB_DIR)/include
RAYLIB_LIB		?= $(RAYLIB_DIR)/lib/libraylib.a

CXXFLAGS_GFX	= -Wall -Wextra -std=c++17 -O2 -MMD -MP -Izappy_gfx/src -isystem $(RAYLIB_INCLUDE)

CC				= gcc
CFLAGS			= -O2 -Wall

CLIENT_INCLUDES	= -Iclient/srcs -Iclient
SERVER_INCLUDES	= -Iserver/srcs -Iserver/raylib -Iserver

CLIENT_LDFLAGS	=
SERVER_LDFLAGS	= -lssl -lcrypto
RAYLIB_LDFLAGS	= -lGL -lm -lpthread -ldl -lrt -lX11
GFX_LDFLAGS		= $(RAYLIB_LIB) $(RAYLIB_LDFLAGS)

# ==============================================================================
#                               TARGET NAMES
# ==============================================================================
SERVER_NAME		= server/server
CLIENT_NAME		= client/client
GFX_NAME		= zappy_gfx/gfx

OBJ_DIR			= objs

# ==============================================================================
#                                CLIENT SOURCES
# ==============================================================================
CLIENT_SRCS		= client/main.cpp \
				  client/srcs/utilities/MZU.cpp \
				  client/srcs/utilities/BasicString.cpp \
				  client/srcs/Network/Connection.cpp \
				  client/srcs/Game/Elevation.cpp \
				  client/srcs/Game/Protocol.cpp \
				  client/srcs/Game/BroadcastMessenger.cpp \
				  client/srcs/Game/SoundNavigator.cpp \
				  client/srcs/Game/ElevationManager.cpp \
				  client/srcs/Game/StrategyStates.cpp \
				  client/srcs/Game/Strategy.cpp \
				  client/srcs/Game/Client.cpp

CLIENT_HEADERS	= client/srcs/zappy.hpp \
				  client/srcs/Enums.hpp \
				  client/srcs/utilities/MZU.hpp \
				  client/srcs/utilities/BasicString.hpp \
				  client/srcs/Network/Connection.hpp \
				  client/srcs/Game/Elevation.hpp \
				  client/srcs/Game/Protocol.hpp \
				  client/srcs/Game/BroadcastMessenger.hpp \
				  client/srcs/Game/SoundNavigator.hpp \
				  client/srcs/Game/ElevationManager.hpp \
				  client/srcs/Game/IState.hpp \
				  client/srcs/Game/StrategyStates.hpp \
				  client/srcs/Game/Strategy.hpp \
				  client/srcs/Game/Client.hpp

CLIENT_OBJS		= $(addprefix $(OBJ_DIR)/, $(CLIENT_SRCS:.cpp=.o))

# ==============================================================================
#                                SERVER SOURCES
# ==============================================================================
SERVER_SRCS		= server/srcs/main.cpp \
				  server/srcs/Game/Protocole.cpp \
				  server/srcs/Game/Command.cpp \
				  server/srcs/Game/CommandParser.cpp \
				  server/srcs/ServerManager/Connection.cpp \
				  server/srcs/ServerManager/ConnectionAdmin.cpp \
				  server/srcs/ServerManager/ConnectionGame.cpp \
				  server/srcs/ServerManager/ConnectionGui.cpp \
				  server/srcs/Game/Egg.cpp \
				  server/srcs/Game/Elevation.cpp \
				  server/srcs/Game/Game.cpp \
				  server/srcs/Game/Player.cpp \
				  server/srcs/Game/Tile.cpp \
				  server/srcs/Game/World.cpp \
				  server/srcs/ServerManager/ServerAdmin.cpp \
				  server/srcs/ServerManager/Core.cpp \
				  server/srcs/ServerManager/ServerGame.cpp \
				  server/srcs/ServerManager/ServerGui.cpp \
				  server/srcs/ServerManager/Server.cpp \
				  server/srcs/ServerManager/ServerManager.cpp \
				  server/srcs/utilities/BasicString.cpp \
				  server/srcs/utilities/MZU.cpp

SERVER_HEADERS	= server/srcs/zappy.hpp \
				  server/srcs/Game/Protocole.hpp \
				  server/srcs/utilities/MZU.hpp \
				  server/srcs/utilities/BasicString.hpp \
				  server/srcs/Game/Tile.hpp \
				  server/srcs/Game/Player.hpp \
				  server/srcs/Game/Egg.hpp \
				  server/srcs/Game/Command.hpp \
				  server/srcs/Game/CommandParser.hpp \
				  server/srcs/Game/Elevation.hpp \
				  server/srcs/Game/World.hpp \
				  server/srcs/Game/Game.hpp \
				  server/srcs/ServerManager/Connection.hpp \
				  server/srcs/ServerManager/ConnectionAdmin.hpp \
				  server/srcs/ServerManager/ConnectionGame.hpp \
				  server/srcs/ServerManager/ConnectionGui.hpp \
				  server/srcs/ServerManager/Server.hpp \
				  server/srcs/ServerManager/ServerGame.hpp \
				  server/srcs/ServerManager/ServerAdmin.hpp \
				  server/srcs/ServerManager/ServerGui.hpp \
				  server/srcs/ServerManager/Core.hpp \
				  server/srcs/ServerManager/ServerManager.hpp

SERVER_OBJS		= $(addprefix $(OBJ_DIR)/, $(SERVER_SRCS:.cpp=.o))

# ==============================================================================
#                               GFX (3D CLIENT) SOURCES
# ==============================================================================
GFX_SRCS		:= $(shell find zappy_gfx/src -name '*.cpp' 2>/dev/null)
GFX_OBJS		:= $(addprefix $(OBJ_DIR)/, $(GFX_SRCS:.cpp=.o))
GFX_DEPS		:= $(GFX_OBJS:.o=.d)

# ==============================================================================
#                                   RULES
# ==============================================================================
all: $(SERVER_NAME) $(CLIENT_NAME) $(GFX_NAME)

# 1. Server Rule
server: $(SERVER_NAME)

certs/server.crt:
	@mkdir -p server/certs certs
	@openssl req -x509 -newkey rsa:2048 -keyout server/certs/server.key -out server/certs/server.crt -days 365 -nodes -subj "/CN=127.0.0.1" -addext "subjectAltName=IP:127.0.0.1,DNS:localhost"
	@cp server/certs/server.* certs/
	@echo "Generated server certificates in server/certs/ and certs/"

certs: certs/server.crt

$(SERVER_NAME): $(SERVER_OBJS) | certs/server.crt
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $(SERVER_NAME) $(SERVER_LDFLAGS)
	@echo "Successfully compiled server: $(SERVER_NAME)"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(SERVER_INCLUDES) $(CLIENT_INCLUDES) -c $< -o $@
	@echo "Compiled $<"

# 2. AI Client Rule
client: $(CLIENT_NAME)

$(CLIENT_NAME): $(CLIENT_OBJS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) -o $(CLIENT_NAME) $(CLIENT_LDFLAGS)
	@echo "Successfully compiled client: $(CLIENT_NAME)"

# 3. GFX 3D Client Rule (zappy_gfx/gfx)
gfx: $(GFX_NAME)

$(GFX_NAME): $(GFX_OBJS) $(RAYLIB_LIB)
	@mkdir -p $(dir $@)
	@$(CXX) $(GFX_OBJS) -o $(GFX_NAME) $(GFX_LDFLAGS)
	@echo "Successfully compiled GFX client: $(GFX_NAME)"

$(OBJ_DIR)/zappy_gfx/%.o: zappy_gfx/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS_GFX) -c $< -o $@
	@echo "Compiled $<"

-include $(GFX_DEPS)

# Helper Rules
bonus: all

connect:
	@ncat --ssl --ssl-verify --ssl-trustfile certs/server.crt localhost 9001

# Cleanup Rules
clean:
	@rm -rf $(OBJ_DIR) zappy_gfx/obj
	@rm -f $(CLIENT_SRCS:.cpp=.o) $(SERVER_SRCS:.cpp=.o)
	@echo "Cleaned all object files."

fclean: clean
	@rm -rf $(SERVER_NAME) server/zappy $(CLIENT_NAME) $(GFX_NAME)
	@rm -rf certs server/certs logs
	@rm -f *.log server/*.log client/*.log
	@echo "Cleaned all executables, certificates, and log files."

re: fclean all

.PHONY: all server client gfx bonus certs connect clean fclean re
