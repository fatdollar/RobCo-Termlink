#include "main.h"
#include "input.h"
#include "external.h"
#include "glob.h"
#include "jobs.h"
#include "builtin.h"
#include "terminal.h"
#include "tl.h"

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

static tl_word_t *_tl_split_fields(const char *s);
static int _tl_run_command_subst(const char *cmd, char *out, size_t out_sz);
static char *_tl_expand_word(const char *s);
static tl_word_t *_tl_expand_all_words(tl_word_t *in);
static int _tl_apply_redirs(tl_redir_t *r);
static void _tl_apply_assignments(tl_word_t *assignments);
static int _tl_exec_builtin(tl_command_t *cmd, tl_exec_ctx_t *ctx);
static char *_tl_build_command_string(tl_command_t *cmd);
static int _tl_exec_node(tl_node_t *node, tl_exec_ctx_t *ctx);
static int _tl_exec_pipeline(tl_pipeline_t *pl, tl_exec_ctx_t *ctx);
static int _tl_exec_list(tl_list_t *list, tl_exec_ctx_t *ctx);
static int _tl_exec_subshell(tl_subshell_t *sub, tl_exec_ctx_t *ctx);

/* ============================================================
 * INTERNAL HELPERS
 * ============================================================ */

static tl_word_t *_tl_split_fields(const char *s)
{
	LOG("_tl_split_fields: input='%s'", s);

	tl_word_t *head = NULL;
	tl_word_t *tail = NULL;

	const char *p = s;

	while (*p)
	{
		/* Skip leading IFS */
		while (*p == ' ' || *p == '\t' || *p == '\n')
			p++;

		if (!*p)
			break;

		char buf[PATH_MAX];
		size_t bi = 0;

		/* Collect one field */
		while (*p &&
			   *p != ' ' &&
			   *p != '\t' &&
			   *p != '\n')
		{
			buf[bi++] = *p++;
			if (bi >= sizeof(buf) - 1)
				break;
		}

		buf[bi] = '\0';

		LOG("_tl_split_fields: field='%s'", buf);

		tl_word_t *w = calloc(1, sizeof(tl_word_t));
		w->text = strdup(buf);

		if (!head)
			head = w;
		else
			tail->next = w;

		tail = w;
	}

	LOG("_tl_split_fields: done");
	return head;
}

static int _tl_run_command_subst(const char *cmd, char *out, size_t out_sz)
{
	LOG("_tl_run_command_subst: cmd='%s'", cmd);

	int pipefd[2];
	if (pipe(pipefd) < 0)
	{
		LOG("_tl_run_command_subst: pipe failed");
		return -1;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		LOG("_tl_run_command_subst: fork failed");
		close(pipefd[0]);
		close(pipefd[1]);
		return -1;
	}

	if (pid == 0)
	{
		/* CHILD: stdout -> pipe, run /bin/sh -c cmd */
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);

		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		_exit(127);
	}

	/* PARENT */
	close(pipefd[1]);

	ssize_t n;
	size_t off = 0;
	while ((n = read(pipefd[0], out + off, out_sz - off - 1)) > 0)
	{
		off += (size_t)n;
		if (off >= out_sz - 1)
			break;
	}
	close(pipefd[0]);

	int status = 0;
	waitpid(pid, &status, 0);

	out[off] = '\0';

	/* strip trailing newlines */
	while (off > 0 && (out[off - 1] == '\n' || out[off - 1] == '\r'))
	{
		out[off - 1] = '\0';
		off--;
	}

	LOG("_tl_run_command_subst: result='%s'", out);
	return 0;
}

static char *_tl_expand_word(const char *s)
{
	if (!strchr(s, '$'))
		return strdup(s);

	LOG("_tl_expand_word: expanding '%s'", s);

	char out[PATH_MAX];
	size_t oi = 0;

	for (size_t i = 0; s[i] != '\0'; i++)
	{
		if (s[i] == '$')
		{
			/* $(command) */
			if (s[i + 1] == '(')
			{
				size_t start = i + 2;
				size_t j = start;
				int depth = 1;

				while (s[j] && depth > 0)
				{
					if (s[j] == '(')
						depth++;
					else if (s[j] == ')')
						depth--;
					j++;
				}

				if (depth != 0)
				{
					LOG("_tl_expand_word: unmatched $('('");
					out[oi++] = '$';
					continue;
				}

				size_t len = j - 1 - start;
				char cmd[PATH_MAX];
				if (len >= sizeof(cmd))
					len = sizeof(cmd) - 1;

				memcpy(cmd, s + start, len);
				cmd[len] = '\0';

				char subst[PATH_MAX];
				if (_tl_run_command_subst(cmd, subst, sizeof(subst)) == 0)
				{
					size_t sl = strlen(subst);
					if (oi + sl >= sizeof(out))
						sl = sizeof(out) - oi - 1;

					memcpy(out + oi, subst, sl);
					oi += sl;
				}

				i = j - 1; /* skip entire $(...) */
				continue;
			}

			/* ${VAR} */
			if (s[i + 1] == '{')
			{
				const char *start = s + i + 2;
				const char *end = strchr(start, '}');
				if (!end)
				{
					out[oi++] = s[i];
					continue;
				}

				char var[256];
				size_t len = (size_t)(end - start);
				if (len >= sizeof(var))
					len = sizeof(var) - 1;

				memcpy(var, start, len);
				var[len] = '\0';

				const char *val = getenv(var);
				LOG("_tl_expand_word: ${%s} -> '%s'", var, val ? val : "");

				if (val)
				{
					size_t vl = strlen(val);
					if (oi + vl >= sizeof(out))
						vl = sizeof(out) - oi - 1;
					memcpy(out + oi, val, vl);
					oi += vl;
				}

				i = (size_t)(end - s);
				continue;
			}

			/* $VAR */
			const char *start = s + i + 1;
			size_t j = 0;
			char var[256];

			while (start[j] && (isalnum((unsigned char)start[j]) || start[j] == '_'))
			{
				var[j] = start[j];
				j++;
			}
			var[j] = '\0';

			if (j == 0)
			{
				out[oi++] = '$';
				continue;
			}

			const char *val = getenv(var);
			LOG("_tl_expand_word: $%s -> '%s'", var, val ? val : "");

			if (val)
			{
				size_t vl = strlen(val);
				if (oi + vl >= sizeof(out))
					vl = sizeof(out) - oi - 1;
				memcpy(out + oi, val, vl);
				oi += vl;
			}

			i += j;
			continue;
		}

		out[oi++] = s[i];
		if (oi >= sizeof(out) - 1)
			break;
	}

	out[oi] = '\0';
	return strdup(out);
}

static tl_word_t *_tl_expand_all_words(tl_word_t *in)
{
	LOG("_tl_expand_all_words: start");

	tl_word_t *out_head = NULL;
	tl_word_t *out_tail = NULL;

	for (tl_word_t *w = in; w; w = w->next)
	{
		LOG("_tl_expand_all_words: expanding '%s'", w->text);

		char *expanded = _tl_expand_word(w->text);
		LOG("_tl_expand_all_words: expanded='%s'", expanded);

		tl_word_t *fields = _tl_split_fields(expanded);
		free(expanded);

		for (tl_word_t *f = fields; f; f = f->next)
		{
			LOG("_tl_expand_all_words: field='%s'", f->text);

			tl_word_t *nw = calloc(1, sizeof(tl_word_t));
			nw->text = strdup(f->text);

			if (!out_head)
				out_head = nw;
			else
				out_tail->next = nw;

			out_tail = nw;
		}
	}

	LOG("_tl_expand_all_words: done");
	return out_head;
}

static int _tl_apply_redirs(tl_redir_t *r)
{
	LOG("_tl_apply_redirs: start");
	for (; r; r = r->next)
	{
		int fd = r->fd;
		int newfd;

		switch (r->type)
		{
		case TL_REDIR_OUT:
			LOG("_tl_apply_redirs: TL_REDIR_OUT to '%s'", r->target->text);
			newfd = open(r->target->text,
						 O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
						 0644);
			break;

		case TL_REDIR_OUT_APPEND:
			LOG("_tl_apply_redirs: TL_REDIR_OUT_APPEND to '%s'", r->target->text);
			newfd = open(r->target->text,
						 O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC,
						 0644);
			break;

		case TL_REDIR_IN:
			LOG("_tl_apply_redirs: TL_REDIR_IN from '%s'", r->target->text);
			newfd = open(r->target->text,
						 O_RDONLY | O_CLOEXEC);
			break;

		case TL_REDIR_ERR_TO_OUT:
			LOG("_tl_apply_redirs: TL_REDIR_ERR_TO_OUT");
			return dup2(1, 2) == -1 ? -1 : 0;

		default:
			LOG("_tl_apply_redirs: unknown redir type=%d", r->type);
			return -1;
		}

		if (newfd < 0)
		{
			LOG("_tl_apply_redirs: open failed");
			return -1;
		}

		if (dup2(newfd, fd) < 0)
		{
			LOG("_tl_apply_redirs: dup2 failed");
			close(newfd);
			return -1;
		}

		close(newfd);
	}

	LOG("_tl_apply_redirs: done");
	return 0;
}

static void _tl_apply_assignments(tl_word_t *assignments)
{
	for (tl_word_t *w = assignments; w; w = w->next)
	{
		const char *s = w->text;
		const char *eq = strchr(s, '=');
		if (!eq)
			continue;

		size_t name_len = (size_t)(eq - s);
		if (name_len == 0)
			continue;

		char name[256];
		if (name_len >= sizeof(name))
			name_len = sizeof(name) - 1;

		memcpy(name, s, name_len);
		name[name_len] = '\0';

		const char *value = eq + 1;

		LOG("_tl_apply_assignments: %s='%s'", name, value);
		setenv(name, value, 1);
	}
}

static int _tl_exec_builtin(tl_command_t *cmd, tl_exec_ctx_t *ctx)
{
	(void)ctx;

	int argc = 0;

	for (tl_word_t *w = cmd->argv; w; w = w->next)
		argc++;

	char **argv = calloc(argc + 1, sizeof(char *));

	int i = 0;
	for (tl_word_t *w = cmd->argv; w; w = w->next)
		argv[i++] = w->text;

	argv[i] = NULL;

	LOG("_tl_exec_builtin: argc=%d", argc);

	for (i = 0; i < argc; i++)
		LOG("_tl_exec_builtin: argv[%d]='%s'", i, argv[i]);

	int rc = builtin_try(argc, argv);

	free(argv);

	return rc;
}

static char *_tl_build_command_string(tl_command_t *cmd)
{
	int total_len = 0;
	for (tl_word_t *w = cmd->argv; w; w = w->next)
	{
		total_len += strlen(w->text) + 1;
	}
	char *cmd_str = calloc(total_len > 0 ? total_len : 1, sizeof(char));
	for (tl_word_t *w = cmd->argv; w; w = w->next)
	{
		strcat(cmd_str, w->text);
		if (w->next)
			strcat(cmd_str, " ");
	}
	return cmd_str;
}

/* ============================================================
 * EXECUTION CHAIN
 * ============================================================ */

static int _tl_exec_node(tl_node_t *node, tl_exec_ctx_t *ctx)
{
	LOG("_tl_exec_node: type=%d", node->type);

	switch (node->type)
	{
	case TL_NODE_PIPELINE:
		return _tl_exec_pipeline(node->as.pipeline, ctx);

	case TL_NODE_LIST:
		return _tl_exec_list(node->as.list, ctx);

	case TL_NODE_SUBSHELL:
		return _tl_exec_subshell(node->as.subshell, ctx);

	default:
		ctx->error_fn("Unsupported node type");
		LOG("_tl_exec_node: unsupported node type=%d", node->type);
		return 1;
	}
}

int _tl_exec_pipeline(tl_pipeline_t *pl, tl_exec_ctx_t *ctx)
{
	(void)ctx;
	LOG("_tl_exec_pipeline: start, count=%d, background=%d", pl->count, pl->background);

	int n = pl->count;
	if (n <= 0)
	{
		LOG("_tl_exec_pipeline: empty pipeline");
		return 0;
	}

	/* Step 1: Expand and prepare all commands in the pipeline first */
	for (int i = 0; i < n; i++)
	{
		tl_command_t *cmd = pl->commands[i];

		/* Expand assignments */
		for (tl_word_t *w = cmd->assignments; w; w = w->next)
		{
			char *expanded = _tl_expand_word(w->text);
			LOG("_tl_exec_pipeline: assignment '%s' -> '%s'", w->text, expanded);
			free(w->text);
			w->text = expanded;
		}

		/* Expand argv and apply globbing */
		cmd->argv = _tl_expand_all_words(cmd->argv);
		cmd->argv = tl_glob_expand_on_argv(cmd->argv);

		/* Expand redirection targets */
		for (tl_redir_t *r = cmd->redirs; r; r = r->next)
		{
			char *expanded = _tl_expand_word(r->target->text);
			LOG("_tl_exec_pipeline: redir '%s' -> '%s'", r->target->text, expanded);
			free(r->target->text);
			r->target->text = expanded;
		}
	}

	/* Step 2: If this is a single-command pipeline (n == 1), check for builtins first */
	if (n == 1)
	{
		tl_command_t *cmd = pl->commands[0];
		if (!cmd->argv)
		{
			LOG("_tl_exec_pipeline: pure assignments");
			_tl_apply_assignments(cmd->assignments);
			return 0;
		}

		_tl_apply_assignments(cmd->assignments);

		int builtin_rc = _tl_exec_builtin(cmd, ctx);
		if (builtin_rc != -1)
		{
			LOG("_tl_exec_pipeline: executed as builtin, status=%d", builtin_rc);
			return builtin_rc;
		}
	}

	/* Step 3: Create pipes for multi-command pipelines */
	int pipes[n > 1 ? n - 1 : 1][2];
	for (int i = 0; i < n - 1; i++)
	{
		if (pipe(pipes[i]) < 0)
		{
			LOG("_tl_exec_pipeline: pipe failed");
			return 1;
		}
	}

	/* Step 4: Build job and process structures */
	job_t *j = calloc(1, sizeof(job_t));
	j->state = JOB_RUNNING;
	j->pgid = 0;

	process_t *last_proc = NULL;
	for (int i = 0; i < n; i++)
	{
		tl_command_t *cmd = pl->commands[i];
		process_t *p = calloc(1, sizeof(process_t));
		p->command = _tl_build_command_string(cmd);
		p->completed = false;
		p->stopped = false;

		if (!j->name)
		{
			j->name = strdup(p->command ? p->command : "command");
		}

		if (!j->first_process)
		{
			j->first_process = p;
		}
		else
		{
			last_proc->next = p;
		}
		last_proc = p;
	}

	/* Step 5: Fork all processes in the pipeline */
	process_t *curr_p = j->first_process;
	for (int i = 0; i < n; i++)
	{
		tl_command_t *cmd = pl->commands[i];

		pid_t pid = fork();
		if (pid < 0)
		{
			LOG("_tl_exec_pipeline: fork failed");
			return 1;
		}

		if (pid == 0)
		{
			/* --- CHILD PROCESS --- */
			pid_t child_pid = getpid();
			if (j->pgid == 0)
			{
				j->pgid = child_pid;
			}
			setpgid(child_pid, j->pgid);

			/* Reset signals to default */
			signal(SIGINT, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			signal(SIGTTIN, SIG_DFL);
			signal(SIGTTOU, SIG_DFL);
			signal(SIGCHLD, SIG_DFL);

			_tl_apply_assignments(cmd->assignments);

			int infd = STDIN_FILENO;
			int outfd = STDOUT_FILENO;

			if (i > 0)
			{
				infd = pipes[i - 1][0];
			}
			if (i < n - 1)
			{
				outfd = pipes[i][1];
			}

			tl_jobs_launch_process(curr_p, infd, outfd, STDERR_FILENO, !pl->background, j->pgid);

			/* Close all pipe fds in child */
			for (int k = 0; k < n - 1; k++)
			{
				close(pipes[k][0]);
				close(pipes[k][1]);
			}

			_tl_apply_redirs(cmd->redirs);

			int argc = 0;
			for (tl_word_t *w = cmd->argv; w; w = w->next)
				argc++;

			char **argv = calloc(argc + 1, sizeof(char *));
			int k = 0;
			for (tl_word_t *w = cmd->argv; w; w = w->next)
				argv[k++] = w->text;
			argv[k] = NULL;

			LOG("_tl_exec_pipeline: exec '%s'", argv[0]);
			execvp(argv[0], argv);
			_exit(127);
		}

		/* --- PARENT PROCESS --- */
		curr_p->pid = pid;
		if (j->pgid == 0)
		{
			j->pgid = pid;
			/* CRITICAL: Set terminal foreground group immediately when pgid is established */
			if (!pl->background && termlink_job_control_enabled)
			{
				tcsetpgrp(STDIN_FILENO, j->pgid);
			}
		}
		setpgid(pid, j->pgid);

		curr_p = curr_p->next;
	}

	/* Parent closes all pipe file descriptors */
	for (int i = 0; i < n - 1; i++)
	{
		close(pipes[i][0]);
		close(pipes[i][1]);
	}

	/* Add job to global job list */
	j->next = g_first_job;
	g_first_job = j;
	int last_status = 1;

	/* Step 6: Branch based on Foreground vs Background (&) */
	if (pl->background)
	{
		/* BACKGROUND JOB: Print job notification and return immediately */
		char buf[512];
		snprintf(buf, sizeof(buf), "[1]+  %d\n", (int)j->pgid);
		terminal_slow_print(buf);
		LOG("_tl_exec_pipeline: background job spawned pgid=%d", (int)j->pgid);
		last_status = 0;
	}
	else
	{
		/* FOREGROUND JOB: Bring to foreground, wait, print feedback, and return exit status */
		last_status = tl_jobs_to_foreground(j, 0);
	}

	LOG("_tl_exec_pipeline: done, status=%d", last_status);
	return last_status;
}

int _tl_exec_list(tl_list_t *list, tl_exec_ctx_t *ctx)
{
	LOG("_tl_exec_list: start");
	tl_list_item_t *it = list->head;
	int status = 0;

	while (it)
	{
		LOG("_tl_exec_list: executing node, op_to_next=%d", it->op_to_next);
		status = _tl_exec_node(it->node, ctx);

		if (it->op_to_next == TL_LIST_AND && status != 0)
		{
			LOG("_tl_exec_list: AND short-circuit, status=%d", status);
			break;
		}

		if (it->op_to_next == TL_LIST_OR && status == 0)
		{
			LOG("_tl_exec_list: OR short-circuit, status=%d", status);
			break;
		}

		it = it->next;
	}

	ctx->last_status = status;
	LOG("_tl_exec_list: done, status=%d", status);
	return status;
}

int _tl_exec_subshell(tl_subshell_t *sub, tl_exec_ctx_t *ctx)
{
	LOG("_tl_exec_subshell: start");

	pid_t pid = fork();
	if (pid < 0)
	{
		ctx->error_fn("fork failed");
		LOG("_tl_exec_subshell: fork failed");
		return 1;
	}

	if (pid == 0)
	{
		LOG("_tl_exec_subshell: child executing body");
		int st = _tl_exec_list(sub->body, ctx);
		LOG("_tl_exec_subshell: child exiting with %d", st);
		exit(st);
	}

	int status;
	waitpid(pid, &status, 0);
	int rc = WIFEXITED(status) ? WEXITSTATUS(status) : 1;

	ctx->last_status = rc;
	LOG("_tl_exec_subshell: done, status=%d", rc);
	return rc;
}

/* ============================================================
 * RUNNER
 * ============================================================ */

int tl_exec_line(const char *line, tl_exec_ctx_t *ctx)
{
	LOG("tl_exec_line: parsing '%s'", line);

	/* Tokenize + parse into AST */
	tl_node_t *root = tl_parse(line);
	if (!root)
	{
		LOG("tl_exec_line: parse failed");
		return 1;
	}

	LOG("tl_exec_line: executing AST");
	int st = _tl_exec_node(root, ctx);

	ctx->last_status = st;
	LOG("tl_exec_line: done, status=%d", st);

	return st;
}