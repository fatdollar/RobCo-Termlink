CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

SRC = \
	src/main.c \
	src/terminal.c \
	src/theme.c \
	src/history.c \
	src/builtin.c \
	src/util.c \
	$(wildcard src/input/*.c)

OBJ = $(patsubst src/%.c, build/%.o, $(SRC))

BIN = build/termlink

all: $(BIN)

build:
	mkdir -p build

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ)

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build