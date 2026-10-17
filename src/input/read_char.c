#include "main.h"
#include "input.h"


int input_read_char(void)
{
    LOG("read_char: waiting for getchar()");
    int c = getchar();
    LOG("read_char: got c=%d (0x%02X)", c, c);

    // CTRL+U, CTRL+K, CTRL+Y
    if (c == 21)
    {
        LOG("read_char: CTRL+U");
        return CTRL_U;
    }
    if (c == 11)
    {
        LOG("read_char: CTRL+K");
        return CTRL_K;
    }
    if (c == 25)
    {
        LOG("read_char: CTRL+Y");
        return CTRL_Y;
    }

    // CTRL+Backspace (^H)
    if (c == 8)
    {
        LOG("read_char: CTRL+Backspace (^H)");
        return CTRL_BACKSPACE;
    }

    // Backspace (DEL)
    if (c == 127)
    {
        LOG("read_char: Backspace (DEL)");
        return BACKSPACE;
    }

    // Not an escape sequence
    if (c != 27)
    {
        LOG("read_char: normal char '%c' (0x%02X)", c, c);
        return c;
    }

    LOG("read_char: ESC detected, reading next byte");
    // ESC sequence begins
    int c1 = getchar();
    LOG("read_char: ESC c1=%d (0x%02X)", c1, c1);

    // ALT+Backspace: ESC 127 or ESC ^H
    if (c1 == 127 || c1 == 8)
    {
        LOG("read_char: ALT+Backspace");
        return ALT_BACKSPACE;
    }

    // ALT+D: ESC d
    if (c1 == 'd' || c1 == 'D')
    {
        LOG("read_char: ALT+D");
        return ALT_D;
    }

    // If not ALT, must be ESC [
    if (c1 != '[')
    {
        LOG("read_char: ESC <non-[> → returning ESC");
        ungetc(c1, stdin);
        return 27;
    }

    LOG("read_char: ESC [ detected, reading c2");
    // Parse ESC [ sequences
    int c2 = getchar();
    LOG("read_char: ESC [ c2=%d (0x%02X)", c2, c2);

    // Standard arrows
    if (c2 == 'A')
    {
        LOG("read_char: UP");
        return UP;
    }
    if (c2 == 'B')
    {
        LOG("read_char: DOWN");
        return DOWN;
    }
    if (c2 == 'C')
    {
        LOG("read_char: RIGHT");
        return RIGHT;
    }
    if (c2 == 'D')
    {
        LOG("read_char: LEFT");
        return LEFT;
    }

    // Delete or CTRL+Delete
    if (c2 == '3')
    {
        LOG("read_char: ESC [ 3 ...");
        int c3 = getchar();
        LOG("read_char: c3=%d (0x%02X)", c3, c3);

        if (c3 == '~')
        {
            LOG("read_char: DEL");
            return DEL;
        }

        if (c3 == ';')
        {
            LOG("read_char: ESC [ 3 ; ...");
            int c4 = getchar(); // '5'
            int c5 = getchar(); // '~'
            LOG("read_char: c4=%d c5=%d", c4, c5);

            if (c4 == '5' && c5 == '~')
            {
                LOG("read_char: CTRL+DELETE");
                return CTRL_DELETE;
            }
        }
    }

    // CTRL+Left / CTRL+Right: ESC [ 1 ; 5 D/C
    if (c2 == '1')
    {
        LOG("read_char: ESC [ 1 ...");
        int c3 = getchar(); // ';'
        int c4 = getchar(); // '5'
        int c5 = getchar(); // D or C

        LOG("read_char: c3=%d c4=%d c5=%d", c3, c4, c5);

        if (c3 == ';' && c4 == '5')
        {
            if (c5 == 'D')
            {
                LOG("read_char: CTRL+LEFT");
                return CTRL_LEFT;
            }
            if (c5 == 'C')
            {
                LOG("read_char: CTRL+RIGHT");
                return CTRL_RIGHT;
            }
        }
    }

    // fallback
    LOG("read_char: fallback, pushing back ESC [");
    ungetc(c2, stdin);
    ungetc('[', stdin);
    return 27;
}