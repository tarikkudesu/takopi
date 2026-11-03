CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17
TARGET = camera_lab
SRC = main.cpp

# Preferred: use pkg-config if Raylib provides a package file.
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS := $(shell pkg-config --libs raylib 2>/dev/null)

# Fallback for common Linux installations if pkg-config has no Raylib entry.
ifeq ($(strip $(RAYLIB_LIBS)),)
    RAYLIB_LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(RAYLIB_CFLAGS) $(SRC) -o $(TARGET) $(RAYLIB_LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

re: clean all

.PHONY: all run clean re