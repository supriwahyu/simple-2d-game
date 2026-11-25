CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O2 $(shell sdl2-config --cflags 2>/dev/null || echo -I/usr/include/SDL2)
LDFLAGS := $(shell sdl2-config --libs 2>/dev/null || echo -lSDL2) -lm

TARGET := build/game
SRC := src/main.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build
