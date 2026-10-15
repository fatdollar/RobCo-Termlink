#include "main.h"
#include "terminal.h"
#include "input.h"
#include "builtin.h"
#include "util.h"
#include "theme.h"
#include "history.h"

int main(void) {
    char input_buf[PATH_MAX];

    terminal_setup();
    terminal_update_size();
    terminal_print_header();

	terminal_slow_print_at(terminal_rows, "Accessing system...");
    
	util_load_completions();
    builtin_load_external_from_path();

    terminal_draw_prompt();

    while (1) {
        input_read_line(input_buf, sizeof(input_buf));
        builtin_process(input_buf);
    }

    return 0;
}