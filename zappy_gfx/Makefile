# Zappy - graphical client (gfx)
#
# This repository holds only the graphical client described in the Zappy
# subject (III.12 / bonus 3D client). The server and AI client are separate
# deliverables and are not built from here.

NAME    = gfx

SRC_DIR = src
OBJ_DIR = obj

SRCS    := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS    := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -MMD -MP -I$(SRC_DIR)
LDLIBS   = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $(NAME) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
