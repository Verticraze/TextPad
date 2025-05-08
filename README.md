# VCTEXTPAD

**VCTEXTPAD** is a simple terminal-based text editor written in C. It draws inspiration from editors like `kilo` and serves as a great starting point for learning low-level terminal manipulation, raw mode input handling, and simple UI rendering in a Linux/Unix environment.

## Features

- Raw mode input handling (no buffering, no echo)
- Cursor movement (arrow keys, page up/down, home/end)
- Simple welcome screen
- Terminal screen drawing using escape sequences
- Clean exit using `Ctrl-Q`

## Prerequisites

- GCC or any C compiler
- A Unix-like terminal (Linux, macOS, WSL, etc.)

## Compilation
```bash
gcc -o vctextpad vctextpad.c
```

## Running the Edit
```
./vctextpad
```
## Exiting the Editor
 Press Ctrl-Q to quit.

## Code Structure

  enableRawMode() / disableRawMode() — Setup/restore terminal settings

  editorReadKey() — Read and parse user key input (including escape sequences)

  editorDrawRows() — Renders the welcome message and row lines

  editorRefreshScreen() — Updates the entire screen at once using a buffer

  editorMoveCursor() — Handles cursor movement logic

  editorProcessKeyPress() — Interprets key input and invokes appropriate behavior

## To Do / Coming Soon

  File I/O (open/save files)

  Text insertion and deletion

  Basic syntax highlighting

  Status bar and message bar

## Credits

Based on concepts from kilo by Salvatore Sanfilippo.
