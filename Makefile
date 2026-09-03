# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ooulcaid <ooulcaid.1337@gmail.com>         +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/09/03 23:50:00 by ooulcaid          #+#    #+#              #
#    Updated: 2026/09/03 23:50:00 by ooulcaid         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# ==============================================================================
#                               COMPILER & FLAGS
# ==============================================================================
CXX				= g++
CXXFLAGS		= -Wall -Wextra -Werror -std=c++11 -g -O0 -fsanitize=address

CLIENT_INCLUDES	= -Iclient/srcs -Iclient
SERVER_INCLUDES	= -Iserver/srcs -Iserver/raylib -Iserver

CLIENT_LDFLAGS	=
SERVER_LDFLAGS	= -lssl -lcrypto -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# ==============================================================================
#                               TARGET NAMES
# ==============================================================================
SERVER_NAME		= server/server
CLIENT_NAME		= client/client

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
				  client/srcs/Game/Strategy.cpp \
				  client/srcs/Game/Client.cpp

CLIENT_HEADERS	= client/srcs/zappy.hpp \
				  client/srcs/Enums.hpp \
				  client/srcs/utilities/MZU.hpp \
				  client/srcs/utilities/BasicString.hpp \
				  client/srcs/Network/Connection.hpp \
				  client/srcs/Game/Elevation.hpp \
				  client/srcs/Game/Protocol.hpp \
				  client/srcs/Game/Strategy.hpp \
				  client/srcs/Game/Client.hpp

CLIENT_OBJS		= $(addprefix $(OBJ_DIR)/, $(CLIENT_SRCS:.cpp=.o))

# ==============================================================================
#                                SERVER SOURCES
# ==============================================================================
SERVER_SRCS		= server/srcs/main.cpp \
				  server/srcs/Game/Command.cpp \
				  server/srcs/Game/CommandParser.cpp \
				  server/srcs/Game/Connection.cpp \
				  server/srcs/Game/ConnectionAdmin.cpp \
				  server/srcs/Game/ConnectionGame.cpp \
				  server/srcs/Game/ConnectionGui.cpp \
				  server/srcs/Game/Egg.cpp \
				  server/srcs/Game/Elevation.cpp \
				  server/srcs/Game/Game.cpp \
				  server/srcs/Game/Player.cpp \
				  server/srcs/Game/Tile.cpp \
				  server/srcs/Game/World.cpp \
				  server/srcs/Game/WorldDisplay.cpp \
				  server/srcs/ServerManager/ServerAdmin.cpp \
				  server/srcs/ServerManager/Core.cpp \
				  server/srcs/ServerManager/ServerGame.cpp \
				  server/srcs/ServerManager/ServerGui.cpp \
				  server/srcs/ServerManager/Server.cpp \
				  server/srcs/ServerManager/ServerManager.cpp \
				  server/srcs/utilities/BasicString.cpp \
				  server/srcs/utilities/MZU.cpp

SERVER_HEADERS	= server/srcs/zappy.hpp \
				  server/srcs/utilities/MZU.hpp \
				  server/srcs/utilities/BasicString.hpp \
				  server/srcs/Game/Enums.hpp \
				  server/srcs/Game/Tile.hpp \
				  server/srcs/Game/Player.hpp \
				  server/srcs/Game/Egg.hpp \
				  server/srcs/Game/Command.hpp \
				  server/srcs/Game/CommandParser.hpp \
				  server/srcs/Game/Elevation.hpp \
				  server/srcs/Game/World.hpp \
				  server/srcs/Game/WorldDisplay.hpp \
				  server/srcs/Game/Game.hpp \
				  server/srcs/Game/Connection.hpp \
				  server/srcs/Game/ConnectionAdmin.hpp \
				  server/srcs/Game/ConnectionGame.hpp \
				  server/srcs/Game/ConnectionGui.hpp \
				  server/srcs/ServerManager/Server.hpp \
				  server/srcs/ServerManager/ServerGame.hpp \
				  server/srcs/ServerManager/ServerAdmin.hpp \
				  server/srcs/ServerManager/ServerGui.hpp \
				  server/srcs/ServerManager/Core.hpp \
				  server/srcs/ServerManager/ServerManager.hpp

SERVER_OBJS		= $(addprefix $(OBJ_DIR)/, $(SERVER_SRCS:.cpp=.o))

# ==============================================================================
#                                   RULES
# ==============================================================================
all: $(SERVER_NAME) $(CLIENT_NAME)

# Client compilation
client: $(CLIENT_NAME)

$(CLIENT_NAME): $(CLIENT_OBJS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) -o $(CLIENT_NAME) $(CLIENT_LDFLAGS)
	@echo "Successfully compiled client: $(CLIENT_NAME)"

$(OBJ_DIR)/client/%.o: client/%.cpp $(CLIENT_HEADERS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(CLIENT_INCLUDES) -c $< -o $@
	@echo "Compiled $<"

# Server compilation
server: $(SERVER_NAME)

gfx: $(SERVER_NAME)

$(SERVER_NAME): $(SERVER_OBJS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $(SERVER_NAME) $(SERVER_LDFLAGS)
	@echo "Successfully compiled server: $(SERVER_NAME)"

$(OBJ_DIR)/server/%.o: server/%.cpp $(SERVER_HEADERS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(SERVER_INCLUDES) -c $< -o $@
	@echo "Compiled $<"

# Helper rules
bonus: all

certs:
	@mkdir -p server/certs
	@openssl req -x509 -newkey rsa:2048 -keyout server/certs/server.key -out server/certs/server.crt -days 365 -nodes -subj "/CN=127.0.0.1" -addext "subjectAltName=IP:127.0.0.1,DNS:localhost"
	@echo "Generated server certificates in server/certs/"

connect:
	@ncat --ssl --ssl-verify --ssl-trustfile server/certs/server.crt localhost 4243

# Cleanup rules
clean:
	@rm -rf $(OBJ_DIR)
	@rm -f $(CLIENT_SRCS:.cpp=.o) $(SERVER_SRCS:.cpp=.o)
	@echo "Cleaned all object files."

fclean: clean
	@rm -f $(SERVER_NAME) $(CLIENT_NAME) server/zappy
	@echo "Cleaned all executables."

re: fclean all

.PHONY: clean certs connect
