SRC		=	srcs/main.cpp \
			srcs/Request/Connection.cpp \
			srcs/Request/Headers.cpp \
			srcs/Request/Request.cpp \
			srcs/Response/Cgi.cpp \
			srcs/Response/ErrorResponse.cpp \
			srcs/Response/FileExplorer.cpp \
			srcs/Response/Get.cpp \
			srcs/Response/Post.cpp \
			srcs/Response/Response.cpp \
			srcs/ServerManager/Core.cpp \
			srcs/ServerManager/Location.cpp \
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
			srcs/Request/Connection.hpp \
			srcs/Request/Headers.hpp \
			srcs/Request/Request.hpp \
			srcs/Response/Cgi.hpp \
			srcs/Response/ErrorResponse.hpp \
			srcs/Response/FileExplorer.hpp \
			srcs/Response/Get.hpp \
			srcs/Response/Post.hpp \
			srcs/Response/Response.hpp \
			srcs/ServerManager/Core.hpp \
			srcs/ServerManager/Location.hpp \
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

.PHONY: clean
