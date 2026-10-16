#include "main.h"
#include "history.h"

static void history_init_path(history_entry *hist)
{
	const char *home = getenv("HOME");
	if (!home)
	{
		hist->history_path[0] = '\0';
		return;
	}

	snprintf(hist->history_path, sizeof(hist->history_path), "%s/%s", home, HISTORY_FILE);
}

void history_load(history_entry *hist)
{
	hist->history_count = 0;
	hist->history_index = 0;
	hist->history_edit_saved = 0;

	const char *home = getenv("HOME");
	if (!home)
	{
		hist->history_path[0] = '\0';
		return;
	}

	snprintf(hist->history_path, sizeof(hist->history_path),
			 "%s/%s", home, HISTORY_FILE);

	FILE *fp = fopen(hist->history_path, "r");
	if (!fp)
		return;

	char buf[PATH_MAX];

	while (fgets(buf, sizeof(buf), fp))
	{
		buf[strcspn(buf, "\n")] = '\0';

		if (hist->history_count < HISTORY_MAX && buf[0] != '\0')
		{
			snprintf(hist->history[hist->history_count],
					 PATH_MAX, "%s", buf);
			hist->history_count++;
		}
	}

	fclose(fp);
	hist->history_index = hist->history_count;
}

void history_save(history_entry *hist)
{
	if (hist->history_path[0] == '\0')
		return;

	FILE *fp = fopen(hist->history_path, "w");
	if (!fp)
		return;

	for (int i = 0; i < hist->history_count; i++)
		fprintf(fp, "%s\n", hist->history[i]);

	fclose(fp);
}

void history_add(history_entry *hist, const char *cmd)
{
	if (!cmd || !*cmd)
		return;

	if (hist->history_count > 0 &&
		strcmp(hist->history[hist->history_count - 1], cmd) == 0)
		return;

	if (hist->history_count < HISTORY_MAX)
	{
		snprintf(hist->history[hist->history_count],
				 PATH_MAX, "%s", cmd);
		hist->history_count++;
	}
	else
	{
		for (int i = 1; i < HISTORY_MAX; i++)
			memmove(hist->history[i - 1],
					hist->history[i], PATH_MAX);

		snprintf(hist->history[HISTORY_MAX - 1],
				 PATH_MAX, "%s", cmd);
	}

	hist->history_index = hist->history_count;
	hist->history_edit_saved = 0;
}

void history_prev(history_entry *hist, char *buf, size_t maxlen)
{
	if (!hist->history_edit_saved &&
		hist->history_index >= hist->history_count)
	{
		snprintf(hist->history_edit_buf, PATH_MAX, "%s", buf);
		hist->history_edit_saved = 1;
	}

	if (hist->history_index == hist->history_count)
		hist->history_index = hist->history_count - 1;
	else if (hist->history_index > 0)
		hist->history_index--;

	snprintf(buf, maxlen,
			 "%s", hist->history[hist->history_index]);
}

void history_next(history_entry *hist, char *buf, size_t maxlen)
{
	if (hist->history_index < hist->history_count - 1)
	{
		hist->history_index++;
		snprintf(buf, maxlen, "%s", hist->history[hist->history_index]);
		return;
	}

	if (hist->history_index == hist->history_count - 1)
	{
		hist->history_index = hist->history_count;

		if (hist->history_edit_saved)
		{
			snprintf(buf, maxlen, "%s", hist->history_edit_buf);
		}
		else
		{
			buf[0] = '\0';
		}

		hist->history_edit_saved = 0;
		return;
	}

	if (hist->history_index == hist->history_count)
	{
		return;
	}
}