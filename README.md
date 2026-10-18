# **Termlink — Project Specification & README**

## **Overview**
Termlink is a retro‑styled terminal shell inspired by the Fallout universe’s RobCo Industries Termlink systems.  
It provides a custom UI, scroll region, slow‑print rendering, command execution pipeline, tab‑completion, and built‑in commands — all implemented in pure C with ANSI escape sequences.

The project is modularized into:

```
src/
    main.c
    terminal.c
    theme.c
    input.c
    history.c
    builtin.c
    pager.c
    external.c

include/
    main.h
    terminal.h
    theme.h
    input.h
    history.h
    builtin.h
    pager.h
    external.h
```

This README documents the current behavior and architecture.

---

## **Features**

### **1. Retro Terminal UI**
- Fallout‑style header banner.
- Fixed scroll region for command output.
- Prompt always drawn at the bottom row.
- Cursor positioning controlled via ANSI escape sequences.
- Slow‑print effect for all scroll‑region output (`terminal_slow_print`, `terminal_slow_print_at`).
- All pager output is slow‑printed (required for aesthetic consistency).

---

### **2. Scroll Region**
- Output is restricted to a vertical region between `terminal_scroll_top` and `terminal_scroll_bottom`.
- `terminal_scroll_clear()` wipes the region and resets the scroll cursor.
- Built‑ins, external commands, and pager output all slow‑print inside this region.
- Scroll region height is fixed at runtime based on terminal size.

---

### **3. Built‑in Commands**

| Command | Description |
|---------|-------------|
| `help`  | Lists built‑ins and external commands (paged, slow‑printed) |
| `clear` | Clears the scroll region |
| `exit`  | Exits Termlink |
| `cd`    | Changes directory |
| `export NAME=VALUE` | Sets environment variable |
| `unset NAME` | Removes environment variable |

Built‑ins are dispatched through `builtin_try()`.

---

### **4. External Commands**
- Executed via `execvp()` inside a forked child.
- STDOUT and STDERR are piped back into Termlink.
- Output is slow‑printed into the scroll region.
- External commands use the **interactive pager loop**, which:
  - decodes ESC sequences (`ESC [ A/B/C/D`)
  - handles UP/DOWN/SPACE/ENTER
  - clears and re‑renders pages
  - redraws the prompt on exit

---

### **5. Pager System**
Termlink includes a full paging engine:

- Slow‑prints each page into the scroll region.
- Supports:
  - `DOWN` / `SPACE` → next page  
  - `UP` → previous page  
  - `ENTER` → advance one line  
  - `q` → quit pager  
- ESC‑sequence decoding is required for arrow keys:
  - `ESC [ A` → UP  
  - `ESC [ B` → DOWN  
- Built‑ins now use the same interactive pager loop as external commands.

---

### **6. Tab Completion**
Supports:

- Directory completion
- File completion
- Command‑specific completion types

Completion sources:

- `/usr/share/termlink/completions`
- `/etc/termlink/completions`
- `$HOME/.termlink/completions`

Completion files end in `.comp` and may contain:

```
command: NAME
type: files | dirs | files_and_dirs | executables | dynamic
extensions: txt, md
options:
  - -n
dynamic: helper_script.sh
```

Completion entries are stored in:

```
completion_entry input_completion_table[MAX_COMPLETIONS];
int input_completion_count;
```

---

### **7. PATH Scanning**
External commands are discovered by scanning each directory in `$PATH`:

- Only executable regular files are registered.
- Names are stored in `external_cmds[]`.

---

### **8. Input System**
Handles:

- Argument parsing (`input_parse_args`)
- Tab completion (`input_handle_tab`)
- Path completion (`input_complete_path`)
- Line editing (backspace, tab, normal characters)
- ESC‑sequence decoding for arrow keys
- Prompt redraw after input

---

### **9. History System (stubbed)**
Currently implemented as empty functions:

```
void history_add(const char *cmd) {}
void history_prev(char *buf, size_t maxlen) {}
void history_next(char *buf, size_t maxlen) {}
```

Ready for future expansion.

---

## **Architecture**

### **main.c**
- Initializes terminal.
- Prints header.
- Loads completions.
- Loads external commands.
- Enters main input loop.

### **terminal.c**
Responsible for:

- Terminal setup and teardown
- Raw mode configuration
- Window size detection
- Scroll region management
- Prompt drawing
- Slow printing
- ESC‑sequence handling for cursor movement

### **pager.c**
Implements:

- Pager buffer
- Page slicing
- ESC‑aware interactive pager loop
- Slow‑print rendering of pages

### **input.c**
Handles:

- Argument parsing
- Tab completion
- Path completion
- Line editing
- ESC‑sequence decoding
- Prompt redraw

### **builtin.c**
Implements:

- Built‑in commands
- Built‑in paging (now using shared pager loop)
- PATH scanning
- Completion registration

### **external.c**
Implements:

- External command execution
- Pipe reading
- Paged output

### **history.c**
Implements a full command history system:

- `history_add(const char *cmd)`  
  Stores commands in an in‑memory ring buffer and persists them to `~/.termlink_history`.

- `history_prev(char *buf, size_t maxlen)`  
  Retrieves the previous history entry (used when pressing ↑).

- `history_next(char *buf, size_t maxlen)`  
  Retrieves the next history entry (used when pressing ↓).

- `history_load(void)`  
  Loads history entries from the history file at startup.

- `history_save(void)`  
  Writes the current history buffer to disk on exit.

The history system supports:

- Consecutive duplicate suppression  
- Navigation state (`history_index`)  
- Persistent storage  
- Integration with the line editor  
- Safe buffer handling (`PATH_MAX`, `HISTORY_MAX`)

---

## **Build Instructions**

### **Requirements**
- GCC or Clang
- POSIX‑compatible system (Linux, BSD, macOS)
- ANSI‑compatible terminal

### **Build**
```
make
```

### **Run**
```
./termlink
```

---

## **Completions Directory Format**

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

---

## **Known Issues / Current State**
- Some ANSI cursor movements may flicker depending on terminal.
- Scroll region behavior is stable but can be improved for long outputs.
- External command output is slow‑printed; large outputs may feel sluggish.
- Completion system does not yet support multi‑argument completion.
- `cd` does not support `~` (HOME).
- Tab completion should not redraw prompt unless text is being added.
- Directory tab completions should append `/` so multiple tab presses work.
- Pager slow‑print can cause minor cursor jitter on some terminals.
- ESC‑sequence decoding is minimal (arrow keys only).

---

## **Planned Features**
- Searchable history (`Ctrl+R`)
- Configurable themes
- Configurable scroll region size
- Command aliases
- Built‑in scripting support
- Custom themes and default theme (`/usr/share/termlink/`)
- Full VT100 key decoding (Home, End, PgUp, PgDn)
- Multi‑argument tab completion

---
