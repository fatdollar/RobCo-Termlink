# Termlink — Project Specification & README

## Overview
Termlink is a retro-styled terminal shell inspired by the Fallout universe’s RobCo Industries Termlink systems.  
It provides a custom UI, scroll region, command execution pipeline, tab-completion, and built-in commands — all implemented in pure C with ANSI escape sequences.

The project is now modularized into multiple source files:

```
src/
    main.c
    terminal.c
    theme.c
    input.c
    history.c
    builtin.c
    util.c

include/
    main.h
    terminal.h
    theme.h
    input.h
    history.h
    builtin.h
    util.h
```

This README documents the current behavior and architecture.

## Features

### 1. Retro Terminal UI
- Fallout-style header banner.
- Fixed scroll region for command output.
- Prompt always drawn at the bottom row.
- Cursor positioning controlled via ANSI escape sequences.
- Slow-print effect for text output (`terminal_slow_print` / `terminal_slow_putc`).

### 2. Scroll Region
- Output is restricted to a vertical region between `terminal_scroll_top` and `terminal_scroll_bottom`.
- `terminal_clear_scroll()` wipes the region and resets the cursor line.
- External commands and built-ins print inside this region.

### 3. Built-in Commands

| Command | Description |
|---------|-------------|
| `help`  | Lists built-ins and external commands |
| `clear` | Clears the scroll region |
| `exit`  | Exits Termlink |
| `cd`    | Changes directory |
| `export NAME=VALUE` | Sets environment variable |
| `unset NAME` | Removes environment variable |

Built-ins are dispatched through `builtin_try()`.

### 4. External Commands
- Executed via `execvp()` inside a forked child.
- STDOUT and STDERR are piped back into Termlink.
- Output is slow-printed into the scroll region.
- Prompt is redrawn after execution.

### 5. Tab Completion
Completion system supports:

- Directory completion
- File completion
- Command-specific completion types

Completion sources:

- `/usr/share/termlink/completions`
- `/etc/termlink/completions`
- `$HOME/.termlink/completions`

Completion files end in `.comp` and may contain:

```
type=dirs
type=files
type=files_and_dirs
```

Completion entries are stored in:

```
completion_entry input_completion_table[MAX_COMPLETIONS];
int input_completion_count;
```

### 6. PATH Scanning
External commands are discovered by scanning each directory in `$PATH`:

- Only executable regular files are registered.
- Names are stored in `builtin_external_cmds`.

### 7. Input System
Handles:

- Parsing arguments (`input_parse_args`)
- Tab completion (`input_handle_tab`)
- Path completion (`input_complete_path`)
- Line editing (backspace, tab, normal characters)

### 8. History System (stubbed)
Currently implemented as empty functions:

```
void history_add(const char *cmd) {}
void history_prev(char *buf, size_t maxlen) {}
void history_next(char *buf, size_t maxlen) {}
```

Ready for future expansion.

## Architecture

### main.c
- Initializes terminal.
- Prints header.
- Loads completions.
- Loads external commands.
- Enters main input loop.

### terminal.c
Responsible for:

- Terminal setup and teardown
- Raw mode configuration
- Window size detection
- Scroll region management
- Prompt drawing
- Slow printing

### theme.c
Applies color themes (currently minimal).

### input.c
Handles:

- Argument parsing
- Tab completion
- Path completion
- Line editing
- Prompt redraw after input

### builtin.c
Implements:

- Built-in commands
- External command execution
- PATH scanning
- Completion registration

### util.c
Implements:

- Executable detection
- Completion file loading
- Completion type parsing

### history.c
Stubbed for future history support.

## Build Instructions

### Requirements
- GCC or Clang
- POSIX-compatible system (Linux, BSD, macOS)
- ANSI-compatible terminal

### Build
```
make
```

### Run
```
./termlink
```

## Completions Directory Format

Place `.comp` files in:

- `/usr/share/termlink/completions`
- `/etc/termlink/completions`
- `$HOME/.termlink/completions`

Example:

**ls.comp**
```
type=files
```

**cd.comp**
```
type=dirs
```

## Known Issues / Current State
- Some ANSI cursor movements may flicker depending on terminal.
- Scroll region behavior is stable but can be improved for long outputs.
- External command output is slow-printed; large outputs may feel sluggish.
- Completion system does not yet support multi-argument completion.
- Command cd doesn't support ~ (HOME)
- Tab completion should not redraw prompt unless text is being added to the prompt area
- Directory tab completions should append / so that multiple tab presses can work

## Planned Features
- Searchable history (`Ctrl+R`)
- Configurable themes
- Configurable scroll region size
- Command aliases
- Built-in scripting support
- Custom Themes and default theme (`/usr/share/termlink/`)