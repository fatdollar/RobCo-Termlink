#include "main.h"
#include "theme.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* ============================================================
 * STATE
 * ============================================================ */

// Default Fallout 4 Pip-Boy True-Color Values
static int fg_r = 0, fg_g = 255, fg_b = 0; // Mint text
static int bg_r = 0, bg_g = 51, bg_b = 0;  // Dull green background
static char cursor_hex[16] = "#00FF00";
static char current_preset[32] = "green";

/* ============================================================
 * HELPERS (Hex Parsing & Scheme Loading)
 * ============================================================ */

static void _parse_hex_color(const char *hex, int *r, int *g, int *b)
{
	if (hex[0] == '#')
		hex++;
	unsigned int val = 0;
	sscanf(hex, "%x", &val);
	*r = (val >> 16) & 0xFF;
	*g = (val >> 8) & 0xFF;
	*b = val & 0xFF;
}

static bool _load_theme_scheme(const char *theme_name)
{
	char path[PATH_MAX];
	const char *home = getenv("HOME");
	FILE *f = NULL;

	// 1. Check user themes directory: ~/.config/termlink/themes/<name>.theme
	if (home)
	{
		snprintf(path, sizeof(path), "%s/.config/termlink/themes/%s.theme", home, theme_name);
		f = fopen(path, "r");
	}

	// 2. Check system themes directory: /usr/share/termlink/themes/<name>.theme
	if (!f)
	{
		snprintf(path, sizeof(path), "/usr/share/termlink/themes/%s.theme", theme_name);
		f = fopen(path, "r");
	}

	if (!f)
	{
		LOG("theme: could not find external .theme file for '%s'", theme_name);
		return false;
	}

	LOG("theme: loading custom scheme from '%s'", path);
	char line[256];
	while (fgets(line, sizeof(line), f))
	{
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
			continue;

		char key[64], val[64];
		if (sscanf(line, "%63[^=]=%63s", key, val) == 2)
		{
			if (strcmp(key, "ColorForeground") == 0)
			{
				_parse_hex_color(val, &fg_r, &fg_g, &fg_b);
			}
			else if (strcmp(key, "ColorBackground") == 0)
			{
				_parse_hex_color(val, &bg_r, &bg_g, &bg_b);
			}
			else if (strcmp(key, "ColorCursor") == 0)
			{
				strncpy(cursor_hex, val, sizeof(cursor_hex) - 1);
				cursor_hex[sizeof(cursor_hex) - 1] = '\0';
			}
		}
	}
	fclose(f);
	return true;
}

/* ============================================================
 * SETUP & LOAD
 * ============================================================ */

void theme_load_config(void)
{
	char path[PATH_MAX];
	const char *home = getenv("HOME");
	FILE *f = NULL;

	// 1. Try user config option A: ~/.config/termlink/termlink.conf
	if (home)
	{
		snprintf(path, sizeof(path), "%s/.config/termlink/termlink.conf", home);
		LOG("theme: trying path '%s'", path);
		f = fopen(path, "r");
	}

	// 2. Try user config option B: ~/.termlinkrc (classic dotfile style)
	if (!f && home)
	{
		snprintf(path, sizeof(path), "%s/.termlinkrc", home);
		LOG("theme: trying path '%s'", path);
		f = fopen(path, "r");
	}

	// 3. Try system-wide fallback: /etc/termlink.conf
	if (!f)
	{
		snprintf(path, sizeof(path), "/etc/termlink.conf");
		LOG("theme: trying path '%s'", path);
		f = fopen(path, "r");
	}

	// If no config file exists anywhere, fall back to hardcoded defaults
	if (!f)
	{
		LOG("theme: no config file found, using default true-color theme");
		strcpy(current_preset, "green");
		theme_apply();
		return;
	}

	LOG("theme: successfully opened config from '%s'", path);

	char line[256];
	bool loaded_custom_scheme = false;

	while (fgets(line, sizeof(line), f))
	{
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
			continue;

		char key[64], val[64];
		if (sscanf(line, "%63[^=]=%63s", key, val) == 2)
		{
			if (strcmp(key, "preset") == 0)
			{
				strcpy(current_preset, val);

				if (strcmp(val, "amber") == 0)
				{
					fg_r = 255;
					fg_g = 176;
					fg_b = 0;
					bg_r = 51;
					bg_g = 34;
					bg_b = 0;
					strcpy(cursor_hex, "#FFB000");
				}
				else if (strcmp(val, "white") == 0)
				{
					fg_r = 220;
					fg_g = 220;
					fg_b = 220;
					bg_r = 30;
					bg_g = 30;
					bg_b = 30;
					strcpy(cursor_hex, "#DCDCDC");
				}
				else if (strcmp(val, "green") == 0)
				{
					fg_r = 0;
					fg_g = 255;
					fg_b = 0;
					bg_r = 0;
					bg_g = 51;
					bg_b = 0;
					strcpy(cursor_hex, "#00FF00");
				}
				else
				{
					// Attempt to load as an external .theme file name (e.g. fallout)
					if (_load_theme_scheme(val))
					{
						loaded_custom_scheme = true;
					}
				}
			}
		}
	}
	fclose(f);

	// Apply the loaded theme settings
	theme_apply();
	LOG("theme: configuration applied (custom scheme loaded: %s)", loaded_custom_scheme ? "yes" : "no");
}

/* ============================================================
 * SAVE
 * ============================================================ */

static int _mkdir_p(const char *dir, mode_t mode)
{
	char tmp[PATH_MAX];
	snprintf(tmp, sizeof(tmp), "%s", dir);
	size_t len = strlen(tmp);
	if (len == 0)
		return 0;
	if (tmp[len - 1] == '/')
		tmp[len - 1] = '\0';

	for (size_t i = 0; i < len; i++)
	{
		if (tmp[i] == '/' && i > 0)
		{
			tmp[i] = '\0';
			if (mkdir(tmp, mode) != 0 && errno != EEXIST)
				return -1;
			tmp[i] = '/';
		}
	}
	return mkdir(tmp, mode);
}

void theme_save_config(const char *preset_name)
{
	char dir_path[PATH_MAX];
	char file_path[PATH_MAX];
	const char *home = getenv("HOME");

	if (!home)
	{
		LOG("theme: error saving config, HOME environment variable not set");
		return;
	}

	// Ensure ~/.config/termlink exists
	snprintf(dir_path, sizeof(dir_path), "%s/.config/termlink", home);
	if (_mkdir_p(dir_path, 0755) != 0 && errno != EEXIST)
	{
		LOG("theme: failed to create config directory %s", dir_path);
		return;
	}

	snprintf(file_path, sizeof(file_path), "%s/termlink.conf", dir_path);
	FILE *f = fopen(file_path, "w");
	if (!f)
	{
		LOG("theme: failed to open %s for writing", file_path);
		return;
	}

	// Update internal state values based on preset
	if (strcmp(preset_name, "amber") == 0)
	{
		fg_r = 255;
		fg_g = 176;
		fg_b = 0;
		bg_r = 51;
		bg_g = 34;
		bg_b = 0;
		strcpy(cursor_hex, "#FFB000");
		strcpy(current_preset, "amber");
	}
	else if (strcmp(preset_name, "white") == 0)
	{
		fg_r = 220;
		fg_g = 220;
		fg_b = 220;
		bg_r = 30;
		bg_g = 30;
		bg_b = 30;
		strcpy(cursor_hex, "#DCDCDC");
		strcpy(current_preset, "white");
	}
	else if (strcmp(preset_name, "green") == 0)
	{
		fg_r = 0;
		fg_g = 255;
		fg_b = 0;
		bg_r = 0;
		bg_g = 51;
		bg_b = 0;
		strcpy(cursor_hex, "#00FF00");
		strcpy(current_preset, "green");
	}
	else
	{
		// Try loading custom scheme, fallback to saving name reference
		strcpy(current_preset, preset_name);
		_load_theme_scheme(preset_name);
	}

	// Write configuration file
	fprintf(f, "# Termlink Configuration File\n");
	fprintf(f, "# Preset options: green, amber, white, or custom .theme name\n");
	fprintf(f, "preset = %s\n", current_preset);

	fclose(f);
	LOG("theme: successfully saved preset '%s' to %s", current_preset, file_path);

	// Apply changes immediately
	theme_apply();
}

/* ============================================================
 * DEFAULT
 * ============================================================ */

void theme_apply(void)
{
	// Reset everything
	printf("\x1b[0m");

	// Apply Foreground True-Color (RGB)
	printf("\x1b[38;2;%d;%d;%dm", fg_r, fg_g, fg_b);

	// Apply Background True-Color (RGB)
	printf("\x1b[48;2;%d;%d;%dm", bg_r, bg_g, bg_b);

	// Cursor color (supported in many true-color terminal emulators)
	printf("\x1b]12;%s\x1b\\", cursor_hex);

	// Cursor shape: underline cursor
	printf("\x1b[0 q");

	// Cursor blink and visibility
	printf("\x1b[?12h");
	printf("\x1b[?25h");

	fflush(stdout);
}