#include "main.h"
#include "input.h"

int input_read_char(void)
{
    int c = getchar();

    // CTRL+U, CTRL+K, CTRL+Y
    if (c == 21) return CTRL_U;
    if (c == 11) return CTRL_K;
    if (c == 25) return CTRL_Y;

    // CTRL+Backspace (^H)
    if (c == 8)
        return CTRL_BACKSPACE;

    // Backspace (DEL)
    if (c == 127)
        return BACKSPACE;

    // Not an escape sequence
    if (c != 27)
        return c;

    // ESC sequence begins
    int c1 = getchar();

    // ALT+Backspace: ESC 127 or ESC ^H
    if (c1 == 127 || c1 == 8)
        return ALT_BACKSPACE;

    // ALT+D: ESC d
    if (c1 == 'd' || c1 == 'D')
        return ALT_D;

    // If not ALT, must be ESC [
    if (c1 != '[')
    {
        ungetc(c1, stdin);
        return 27;
    }

    // Parse ESC [ sequences
    int c2 = getchar();

    // Standard arrows
    if (c2 == 'A') return UP;
    if (c2 == 'B') return DOWN;
    if (c2 == 'C') return RIGHT;
    if (c2 == 'D') return LEFT;

    // Delete or CTRL+Delete
    if (c2 == '3')
    {
        int c3 = getchar();
        if (c3 == '~')
            return DEL;

        if (c3 == ';')
        {
            int c4 = getchar(); // '5'
            int c5 = getchar(); // '~'
            if (c4 == '5' && c5 == '~')
                return CTRL_DELETE;
        }
    }

    // CTRL+Left / CTRL+Right: ESC [ 1 ; 5 D/C
    if (c2 == '1')
    {
        int c3 = getchar(); // ';'
        int c4 = getchar(); // '5'
        int c5 = getchar(); // D or C

        if (c3 == ';' && c4 == '5')
        {
            if (c5 == 'D') return CTRL_LEFT;
            if (c5 == 'C') return CTRL_RIGHT;
        }
    }

    // fallback
    ungetc(c2, stdin);
    ungetc('[', stdin);
    return 27;
}