#include "main.h"
#include "util.h"
#include "input.h"
#include "autocomplete.h"

int util_is_executable(const char *path)
{
	LOG("util_is_executable: path='%s'", path);

	struct stat st;
	if (stat(path, &st) != 0)
	{
		LOG("util_is_executable: stat failed");
		return 0;
	}

	if (!S_ISREG(st.st_mode))
	{
		LOG("util_is_executable: not a regular file");
		return 0;
	}

	int exec = (st.st_mode & S_IXUSR) ||
			   (st.st_mode & S_IXGRP) ||
			   (st.st_mode & S_IXOTH);

	LOG("util_is_executable: exec=%d", exec);
	return exec;
}

autocomplete_type util_parse_completion_type(const char *value)
{
	LOG("util_parse_completion_type: value='%s'", value);

	if (strcmp(value, "dirs") == 0)
		return AUTOCOMPLETE_DIRS;
	if (strcmp(value, "files") == 0)
		return AUTOCOMPLETE_FILES;

	LOG("util_parse_completion_type: default");
	return AUTOCOMPLETE_FILES_AND_DIRS;
}

void util_register_completion(char *cmd, autocomplete_type type)
{
	LOG("util_register_completion: cmd='%s' type=%d", cmd, type);

	if (input_autocomplete_count >= MAX_COMPLETIONS)
	{
		LOG("util_register_completion: MAX_COMPLETIONS reached");
		return;
	}

	input_autocomplete_table[input_autocomplete_count].cmd = cmd;

	input_autocomplete_table[input_autocomplete_count].type = type;
	input_autocomplete_count++;

	LOG("util_register_completion: count=%d", input_autocomplete_count);
}

void util_load_completions_dir(const char *dirpath)
{
	LOG("util_load_completions_dir: dir='%s'", dirpath);

	DIR *d = opendir(dirpath);
	if (!d)
	{
		LOG("util_load_completions_dir: cannot open");
		return;
	}

	struct dirent *ent;
	while ((ent = readdir(d)) != NULL)
	{
		LOG("util_load_completions_dir: found '%s'", ent->d_name);

		const char *name = ent->d_name;
		size_t len = strlen(name);

		if (len <= 5)
			continue;
		if (strcmp(name + len - 5, ".comp") != 0)
			continue;

		LOG("util_load_completions_dir: .comp file '%s'", name);

		char cmd[64];
		size_t cmdlen = len - 5;
		if (cmdlen >= sizeof(cmd))
			cmdlen = sizeof(cmd) - 1;
		memcpy(cmd, name, cmdlen);
		cmd[cmdlen] = '\0';

		LOG("util_load_completions_dir: cmd='%s'", cmd);

		char path[PATH_MAX];
		snprintf(path, sizeof(path), "%s/%s", dirpath, name);

		FILE *f = fopen(path, "r");
		if (!f)
		{
			LOG("util_load_completions_dir: cannot open file '%s'", path);
			continue;
		}

		char line[256];
		autocomplete_type type = AUTOCOMPLETE_FILES_AND_DIRS;

		while (fgets(line, sizeof(line), f))
		{
			LOG("util_load_completions_dir: line='%s'", line);

			char *nl = strchr(line, '\n');
			if (nl)
				*nl = '\0';

			if (strncmp(line, "type=", 5) == 0)
			{
				const char *val = line + 5;
				type = util_parse_completion_type(val);
				LOG("util_load_completions_dir: parsed type=%d", type);
				break;
			}
		}

		fclose(f);

		util_register_completion(cmd, type);
	}

	closedir(d);
}

void util_load_completions(void)
{
	LOG("util_load_completions: start");

	util_load_completions_dir("/usr/share/termlink/completions");
	util_load_completions_dir("/etc/termlink/completions");

	const char *home = getenv("HOME");
	LOG("util_load_completions: HOME='%s'", home ? home : "(null)");

	if (home && *home)
	{
		char path[PATH_MAX];
		snprintf(path, sizeof(path), "%s/.termlink/completions", home);
		LOG("util_load_completions: user dir='%s'", path);
		util_load_completions_dir(path);
	}

	LOG("util_load_completions: adding built-in 'cd'");
	util_register_completion("cd", AUTOCOMPLETE_DIRS);

	LOG("util_load_completions: done");
}