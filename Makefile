SRC		=	srcs/main.cpp \
			srcs/Game/Client.cpp \
			srcs/Game/Connection.cpp \
			srcs/Game/Game.cpp \
			srcs/ServerManager/AdminServer.cpp \
			srcs/ServerManager/Core.cpp \
			srcs/ServerManager/GameServer.cpp \
			srcs/ServerManager/GuiServer.cpp \
			srcs/ServerManager/Server.cpp \
			srcs/ServerManager/ServerManager.cpp \
			srcs/utilities/BasicString.cpp \
			srcs/utilities/MZU.cpp
OBJ		=	$(SRC:.cpp=.o)
NAME	=	zappy

all: $(NAME)

$(NAME): $(OBJ)	srcs/zappy.hpp \
			srcs/Game/Client.hpp \
			srcs/Game/Connection.hpp \
			srcs/Game/Game.hpp \
			srcs/ServerManager/AdminServer.hpp \
			srcs/ServerManager/Core.hpp \
			srcs/ServerManager/GameServer.hpp \
			srcs/ServerManager/GuiServer.hpp \
			srcs/ServerManager/Server.hpp \
			srcs/ServerManager/ServerManager.hpp \
			srcs/utilities/BasicString.hpp \
			srcs/utilities/MZU.hpp
	@g++ -Wall -Wextra -Werror -std=c++98 $(OBJ) -o $(NAME)

%.o: %.cpp	srcs/zappy.hpp \
			srcs/Game/Client.hpp \
			srcs/Game/Connection.hpp \
			srcs/Game/Game.hpp \
			srcs/ServerManager/AdminServer.hpp \
			srcs/ServerManager/Core.hpp \
			srcs/ServerManager/GameServer.hpp \
			srcs/ServerManager/GuiServer.hpp \
			srcs/ServerManager/Server.hpp \
			srcs/ServerManager/ServerManager.hpp \
			srcs/utilities/BasicString.hpp \
			srcs/utilities/MZU.hpp
	@g++ -Wall -Wextra -Werror -std=c++98 -c $< -o $@

clean:
	@rm -f $(OBJ)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: clean fclean re all
