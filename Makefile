CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

SRC = src/main.c src/terminal.c src/theme.c src/input.c src/history.c src/builtin.c src/util.c
OBJ = $(patsubst src/%.c, build/%.o, $(SRC))

BIN = build/termlink

# Default target
all: $(BIN)

# Ensure build directory exists
build:
	mkdir -p build

# Compile object files into build/
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# Link final binary
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ)

# Run the program
run: $(BIN)
	./$(BIN)

# Clean build directory
clean:
	rm -rf build