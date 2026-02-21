CXX := g++

CXXFLAGS := -Wall -Wextra -O3 -std=c++20 -Iinclude -I/usr/include/freetype2 -I. `sdl2-config --cflags` -DRAPHENGINE2_EXPORTS -fPIC -fsanitize=address -g

SRC := $(shell find src -name '*.cpp')
OBJ := $(patsubst %.cpp, build/objs/%.o, $(SRC))

TARGET := RaphEngine2

all: $(TARGET)

$(TARGET): $(OBJ)
	ar rcs build/lib$@.a $^
	sudo cp build/lib$@.a /usr/local/lib
	sudo rm -rf /usr/local/include/RaphEngine
	sudo mkdir -p /usr/local/include/RaphEngine
	sudo cp include/*.h /usr/local/include/RaphEngine

build/objs/%.o: %.cpp
	mkdir -p build/objs/src
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build/

.PHONY: all clean
