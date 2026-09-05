SRC		=	srcs/main.cpp \
			srcs/Game/Command.cpp \
			srcs/Game/CommandParser.cpp \
			srcs/Game/Egg.cpp \
			srcs/Game/Elevation.cpp \
			srcs/Game/Game.cpp \
			srcs/Game/Player.cpp \
			srcs/Game/Tile.cpp \
			srcs/Game/World.cpp \
			srcs/Game/WorldDisplay.cpp \
			srcs/ServerManager/ServerAdmin.cpp \
			srcs/ServerManager/Connection.cpp \
			srcs/ServerManager/ConnectionAdmin.cpp \
			srcs/ServerManager/ConnectionGame.cpp \
			srcs/ServerManager/ConnectionGui.cpp \
			srcs/ServerManager/Core.cpp \
			srcs/ServerManager/ServerGame.cpp \
			srcs/ServerManager/ServerGui.cpp \
			srcs/ServerManager/Server.cpp \
			srcs/ServerManager/ServerManager.cpp \
			srcs/utilities/BasicString.cpp \
			srcs/utilities/MZU.cpp
OBJ		=	$(SRC:.cpp=.o)
NAME	=	zappy

HEADERS	=	srcs/zappy.hpp \
			srcs/utilities/MZU.hpp \
			srcs/utilities/BasicString.hpp \
			srcs/Game/Tile.hpp \
			srcs/Game/Player.hpp \
			srcs/Game/Egg.hpp \
			srcs/Game/Command.hpp \
			srcs/Game/CommandParser.hpp \
			srcs/Game/Elevation.hpp \
			srcs/Game/World.hpp \
			srcs/Game/WorldDisplay.hpp \
			srcs/Game/Game.hpp \
			srcs/ServerManager/Connection.hpp \
			srcs/ServerManager/ConnectionAdmin.hpp \
			srcs/ServerManager/ConnectionGame.hpp \
			srcs/ServerManager/ConnectionGui.hpp \
			srcs/ServerManager/Server.hpp \
			srcs/ServerManager/ServerGame.hpp \
			srcs/ServerManager/ServerAdmin.hpp \
			srcs/ServerManager/ServerGui.hpp \
			srcs/ServerManager/Core.hpp \
			srcs/ServerManager/ServerManager.hpp

# -g -O0 -fsanitize=address
CXX		=	g++
CXXFLAGS=	-Wall -Wextra -Werror -std=c++11 -Iraylib
LDFLAGS	=	-lssl -lcrypto -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: $(NAME)

$(NAME): $(OBJ)
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

connect:
	@ncat --ssl --ssl-verify --ssl-trustfile certs/server.crt localhost 4243

certs:
	@mkdir -p certs
	@openssl req -x509 -newkey rsa:2048 -keyout certs/server.key -out certs/server.crt -days 365 -nodes -subj "/CN=127.0.0.1" -addext "subjectAltName=IP:127.0.0.1,DNS:localhost"

clean:
	@rm -f $(OBJ)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: clean fclean re all certs
