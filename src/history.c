#include "main.h"
#include "history.h"

static void _history_init_path(history_entry *hist)
{
	LOG_HISTORY("init_path: HOME='%s'", getenv("HOME"));

	const char *home = getenv("HOME");
	if (!home)
	{
		LOG_HISTORY("init_path: no HOME, disabling history");
		hist->history_path[0] = '\0';
		return;
	}

	snprintf(hist->history_path, sizeof(hist->history_path),
			 "%s/%s", home, HISTORY_FILE);

	LOG_HISTORY("init_path: path='%s'", hist->history_path);
}

void history_load(history_entry *hist)
{
	LOG_HISTORY("load: start");

	hist->history_count = 0;
	hist->history_index = 0;
	hist->history_edit_saved = 0;

	_history_init_path(hist);

	FILE *fp = fopen(hist->history_path, "r");
	if (!fp)
	{
		LOG_HISTORY("load: no history file");
		return;
	}

	char buf[PATH_MAX];

	while (fgets(buf, sizeof(buf), fp))
	{
		buf[strcspn(buf, "\n")] = '\0';
		LOG_HISTORY("load: read='%s'", buf);

		if (hist->history_count < HISTORY_MAX && buf[0] != '\0')
		{
			snprintf(hist->history[hist->history_count],
					 PATH_MAX, "%s", buf);
			LOG_HISTORY("load: stored[%d]='%s'",
						hist->history_count, buf);
			hist->history_count++;
		}
	}

	fclose(fp);

	hist->history_index = hist->history_count;
	LOG_HISTORY("load: count=%d index=%d",
				hist->history_count, hist->history_index);
}

void history_save(history_entry *hist)
{
	LOG_HISTORY("save: path='%s'", hist->history_path);

	if (hist->history_path[0] == '\0')
	{
		LOG_HISTORY("save: no path, skipping");
		return;
	}

	FILE *fp = fopen(hist->history_path, "w");
	if (!fp)
	{
		LOG_HISTORY("save: fopen failed");
		return;
	}

	for (int i = 0; i < hist->history_count; i++)
	{
		LOG_HISTORY("save: writing[%d]='%s'", i, hist->history[i]);
		fprintf(fp, "%s\n", hist->history[i]);
	}

	fclose(fp);
	LOG_HISTORY("save: done");
}

void history_add(history_entry *hist, const char *cmd)
{
	LOG_HISTORY("add: cmd='%s'", cmd);

	if (!cmd || !*cmd)
	{
		LOG_HISTORY("add: empty, skipping");
		return;
	}

	if (hist->history_count > 0 &&
		strcmp(hist->history[hist->history_count - 1], cmd) == 0)
	{
		LOG_HISTORY("add: duplicate of last, skipping");
		return;
	}

	if (hist->history_count < HISTORY_MAX)
	{
		LOG_HISTORY("add: storing at %d", hist->history_count);
		snprintf(hist->history[hist->history_count],
				 PATH_MAX, "%s", cmd);
		hist->history_count++;
	}
	else
	{
		LOG_HISTORY("add: full, shifting");
		for (int i = 1; i < HISTORY_MAX; i++)
			memmove(hist->history[i - 1],
					hist->history[i], PATH_MAX);

		snprintf(hist->history[HISTORY_MAX - 1],
				 PATH_MAX, "%s", cmd);
	}

	hist->history_index = hist->history_count;
	hist->history_edit_saved = 0;

	LOG_HISTORY("add: count=%d index=%d edit_saved=%d",
				hist->history_count,
				hist->history_index,
				hist->history_edit_saved);
}

void history_prev(history_entry *hist, char *buf, size_t maxlen)
{
	LOG_HISTORY("prev: index=%d count=%d edit_saved=%d buf='%s'",
				hist->history_index,
				hist->history_count,
				hist->history_edit_saved,
				buf);

	if (!hist->history_edit_saved &&
		hist->history_index >= hist->history_count)
	{
		LOG_HISTORY("prev: saving edit buffer '%s'", buf);
		snprintf(hist->history_edit_buf, PATH_MAX, "%s", buf);
		hist->history_edit_saved = 1;
	}

	if (hist->history_index == hist->history_count)
	{
		hist->history_index = hist->history_count - 1;
		LOG_HISTORY("prev: moved to last index=%d", hist->history_index);
	}
	else if (hist->history_index > 0)
	{
		hist->history_index--;
		LOG_HISTORY("prev: moved to index=%d", hist->history_index);
	}

	snprintf(buf, maxlen, "%s", hist->history[hist->history_index]);
	LOG_HISTORY("prev: result='%s'", buf);
}

void history_next(history_entry *hist, char *buf, size_t maxlen)
{
	LOG_HISTORY("next: index=%d count=%d edit_saved=%d buf='%s'",
				hist->history_index,
				hist->history_count,
				hist->history_edit_saved,
				buf);

	if (hist->history_index < hist->history_count - 1)
	{
		hist->history_index++;
		LOG_HISTORY("next: moved to index=%d", hist->history_index);
		snprintf(buf, maxlen, "%s", hist->history[hist->history_index]);
		LOG_HISTORY("next: result='%s'", buf);
		return;
	}

	if (hist->history_index == hist->history_count - 1)
	{
		hist->history_index = hist->history_count;
		LOG_HISTORY("next: moved to edit buffer index=%d", hist->history_index);

		if (hist->history_edit_saved)
		{
			LOG_HISTORY("next: restoring edit buffer '%s'", hist->history_edit_buf);
			snprintf(buf, maxlen, "%s", hist->history_edit_buf);
		}
		else
		{
			LOG_HISTORY("next: no edit buffer, clearing");
			buf[0] = '\0';
		}

		hist->history_edit_saved = 0;
		return;
	}

	LOG_HISTORY("next: at end, nothing to do");
}