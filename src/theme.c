#include "main.h"
#include "theme.h"

void theme_apply_default(void)
{
	// Reset everything
	printf("\x1b[0m");

	// Fallout 4 mint text
	printf("\x1b[38;2;0;255;0m");

	// Fallout dull green background
	printf("\x1b[48;2;0;51;0m");

	// Cursor color (supported in many terminals)
	printf("\x1b]12;#00FF00\x1b\\");

	// Cursor shape (supported in some terminals)
	printf("\x1b[0 q"); // underline cursor

	// Cursor blink (supported in most terminals)
	printf("\x1b[?12h");
	printf("\x1b[?25h");
}