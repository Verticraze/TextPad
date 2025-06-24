# VCTEXTPAD

**VCTEXTPAD** is a simple terminal-based text editor written in C. It draws inspiration from editors like `kilo` and serves as a great starting point for learning low-level terminal manipulation, raw mode input handling, and simple UI rendering in a Linux/Unix environment.

## Features
- Editing,Viewing,Saving Text, without leaving the Terminal
- Raw mode input handling (no buffering, no echo)
- Cursor movement (arrow keys, page up/down, home/end)
- Message Bar/Status Bar
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
## Credits

Based on the text editor kilo by Salvatore Sanfilippo.
