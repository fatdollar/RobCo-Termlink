CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

SRC = src/main.c src/terminal.c src/theme.c src/input.c src/history.c src/builtin.c src/util.c
OBJ = $(SRC:.c=.o)

termlink: $(OBJ)
	$(CC) $(CFLAGS) -o termlink $(OBJ)

clean:
	rm -f $(OBJ) termlink