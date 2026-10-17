#include "terminal.h"
#include "builtin.h"

int builtin_help(int argc, char **argv)
{
	terminal_slow_print("Available commands:");

	terminal_slow_print("Built-ins:");
	terminal_slow_print("  help");
	terminal_slow_print("  clear");
	terminal_slow_print("  exit");
	terminal_slow_print("  cd");
	terminal_slow_print("  export");
	terminal_slow_print("  unset");

	terminal_slow_print("");
	terminal_slow_print("External commands:");

	return 1;
}