#define _GNU_SOURCE

#include <termios.h>
#include <sys/ioctl.h>
#include <pty.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>

#include "main.h"
#include "pty.h"
#include "terminal.h"
#include "theme.h"
#include "input.h"

extern pid_t forkpty(int *amaster, char *name, const struct termios *termp, const struct winsize *winp);

int pty_run_interactive(char **argv)
{
	LOG("pty_run_interactive: start, cmd='%s'", argv[0]);

	int master_fd;
	pid_t pid;
	struct winsize ws;

	// Set window size to match your bounded Fallout UI window exactly
	memset(&ws, 0, sizeof(ws));
	ws.ws_row = terminal_rows;
	ws.ws_col = terminal_cols;
	ws.ws_xpixel = 0;
	ws.ws_ypixel = 0;

	// Move cursor to the start of the scrollable viewport (row 5) and restrict scrolling region
	printf("\033[%d;1H", terminal_scroll_top);

	pid = forkpty(&master_fd, NULL, NULL, &ws);
	if (pid < 0)
	{
		LOG("pty_run_interactive: forkpty failed");
		return 1;
	}

	if (pid == 0)
	{
		/* --- CHILD PROCESS --- */
		execvp(argv[0], argv);
		perror("execvp failed");
		_exit(127);
	}

	/* --- PARENT PROCESS --- */
	LOG("pty_run_interactive: parent process bridged with master_fd=%d", master_fd);

	// Hand over terminal foreground control to the child process group so signals work!
	tcsetpgrp(STDIN_FILENO, pid);

	// Clear terminal scroll region before launching the full-screen app
	terminal_scroll_clear();

	fd_set readfds;
	int max_fd = master_fd > STDIN_FILENO ? master_fd : STDIN_FILENO;

	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(master_fd, &readfds);
		FD_SET(STDIN_FILENO, &readfds);

		int ret = select(max_fd + 1, &readfds, NULL, NULL, NULL);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
			break;
		}

		// Data from the interactive program -> write to stdout
		if (FD_ISSET(master_fd, &readfds))
		{
			char buf[4096];
			ssize_t n = read(master_fd, buf, sizeof(buf));
			if (n <= 0)
				break;
			write(STDOUT_FILENO, buf, n);
		}

		// Keyboard input from user -> write to the master PTY
		if (FD_ISSET(STDIN_FILENO, &readfds))
		{
			char buf[4096];
			ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
			if (n <= 0)
				break;
			write(master_fd, buf, n);
		}
	}

	close(master_fd);

	int status = 0;
	waitpid(pid, &status, 0);

	// Take terminal foreground control back to the shell!
	tcsetpgrp(STDIN_FILENO, getpid());

	// Restore terminal state and redraw prompt upon exit
	terminal_scroll_clear();
	theme_apply_default();
	terminal_header_print();
	terminal_prompt_draw();

	int rc = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
	LOG("pty_run_interactive: finished with exit code=%d", rc);
	return rc;
}