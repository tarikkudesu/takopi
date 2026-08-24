SRC		=	srcs/main.cpp \
			srcs/Game/Client.cpp \
			srcs/Game/Connection.cpp \
			srcs/Game/Game.cpp \
			srcs/ServerManager/Core.cpp \
			srcs/ServerManager/Server.cpp \
			srcs/ServerManager/ServerManager.cpp \
			srcs/utilities/BasicString.cpp \
			srcs/utilities/WSU.cpp
OBJ		=	$(SRC:.cpp=.o)
NAME	=	zappy

all: $(NAME)

$(NAME): $(OBJ)
	@g++ -Wall -Wextra -Werror -std=c++98 $(OBJ) -o $(NAME)

%.o: %.cpp	srcs/webserv.hpp \
			srcs/Game/Client.hpp \
			srcs/Game/Connection.hpp \
			srcs/Game/Game.hpp \
			srcs/ServerManager/Core.hpp \
			srcs/ServerManager/Server.hpp \
			srcs/ServerManager/ServerManager.hpp \
			srcs/utilities/BasicString.hpp \
			srcs/utilities/WSU.hpp
	@g++ -Wall -Wextra -Werror -std=c++98 -c $< -o $@

clean:
	@rm -f $(OBJ)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: clean fclean re all
