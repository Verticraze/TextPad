// includes

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#define CTRL_KEY(k) ((k) & 0x1f)

// enum

enum editorKey
{
    ARROW_UP = 1000,

    ARROW_DOWN,

    ARROW_RIGHT,

    ARROW_LEFT,

	DEL_KEY,

    HOME_KEY,

    END_KEY,

    PAGE_UP,

    PAGE_DOWN

};

// data

typedef struct erow
{
	int size;
	
	char *chars;
	
}erow;

struct editorConfig
{
    int cx, cy;
	
	int rowoff;

    int screenrows;

    int screencols;

	int numrows;
	
	erow *row;

    struct termios orig_termios;

};

struct editorConfig E;

// terminal

void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);

    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);

    exit(1);

}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    {

        die("tcsetattr");

    }

}

void enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    {

        die("tcgetattr");

    }

    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

// Turning off all the miscallaneous flags

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    raw.c_oflag &= ~OPOST;

    raw.c_cflag |= (CS8);

    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cc[VMIN] = 0;

    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    {

        die("tcsetattr");

    }

}

int editorReadKey()
{
    int nread;

    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {

        if (nread == -1 && errno != EAGAIN)
        {

            die("read");

        }

    }

    if (c == '\x1b')
    {

        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1 || read(STDIN_FILENO, &seq[1], 1) != 1)
        {

            return '\x1b';

        }

        if (seq[0] == '[')
        {

            if (seq[1] >= '0' && seq[1] <= '9')
            {

                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                {

                    return '\x1b';

                }

                if (seq[2] == '~')
                {

                    switch (seq[1])
                    {

                        case '1': return HOME_KEY;
						
						case '3': return DEL_KEY;

                        case '4': return END_KEY;

                        case '5': return PAGE_UP;

                        case '6': return PAGE_DOWN;

                        case '7': return HOME_KEY;

                        case '8': return END_KEY;

                    }

                }

            }
            else
            {

                switch (seq[1])
                {

                    case 'A': return ARROW_UP;

                    case 'B': return ARROW_DOWN;

                    case 'C': return ARROW_RIGHT;

                    case 'D': return ARROW_LEFT;

                    case 'H': return HOME_KEY;

                    case 'F': return END_KEY;

                }

            }

        }
        else if (seq[0] == 'O')
        {

            switch (seq[1])
            {

                case 'H': return HOME_KEY;

                case 'F': return END_KEY;

            }

        }

        return '\x1b';

    }
    else
    {

        return c;

    }

}

int getCursorPosition(int *rows, int *cols)
{
    char buff[32];

    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    {

        return -1;

    }

    while (i < sizeof(buff) - 1)
    {

        if (read(STDIN_FILENO, &buff[i], 1) != 1)
        {

            break;

        }

        if (buff[i] == 'R')
        {

            break;

        }

        i++;

    }

    buff[i] = '\0';

    if (buff[0] != '\x1b' || buff[1] != '[')
    {

        return -1;

    }

    if (sscanf(&buff[2], "%d;%d", rows, cols) != 2)
    {

        return -1;

    }

    return 0;

}

int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {

        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
        {

            return -1;

        }

        return getCursorPosition(rows, cols);

    }
    else
    {

        *cols = ws.ws_col;

        *rows = ws.ws_row;

        return 0;

    }

}

void editorAppendRow(char *s, size_t len)
{
	E.row = realloc(E.row,sizeof(erow)*(E.numrows+1));
	
	int pos=E.numrows;
	
	E.row[pos].size=len;
	
	E.row[pos].chars=malloc(len+1);
	
	memcpy(E.row[pos].chars,s,len);
	
	E.row[pos].chars[len]='\0';
	
	E.numrows++;
}

void editorOpen(char *filename)
{
	FILE *fp=fopen(filename,"r");
	
	if(!fp) die("fopen");
	
	char *line = NULL;

	size_t linecap=0;
	
	ssize_t linelen;
	
	while((linelen = getline(&line,&linecap,fp)) != -1)
	
	if(linelen!=-1)
	{
		while(linelen > 0 && (line[linelen-1] == '\n' || line[linelen - 1] == '\r'))
		{
			linelen--;
			
			editorAppendRow(line,linelen);
		}
	
	}

	free(line);

	fclose(fp);
}

// append buffer

struct abuf
{

    char *b;

    int len;

};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, char *s, int len)
{

    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL)
    {

        return;

    }

    memcpy(&new[ab->len], s, len);

    ab->b = new;

    ab->len += len;

}

void abFree(struct abuf *ab)
{

    free(ab->b);

}

// output

void editorScroll()
{
	if(E.cy < E.rowoff)
	{
		E.rowoff=E.cy;
	}
	if(E.cy >= E.rowoff + E.screenrows)
	{
		E.rowoff = E.cy - E.screenrows + 1;
	}
}

void editordrawRows(struct abuf *ab)
{

    int y;

    for (y = 0; y < E.screenrows; y++)
    {
		
		int filerows = y + E.rowoff;
		
		if(filerows >= E.numrows)
		{
			if (E.numrows == 0 && y == E.screenrows / 3)
			{

				char welcome[80];

				int welcomelen = snprintf(welcome, sizeof(welcome), "VCTEXTPAD");

				if (welcomelen > E.screencols)
				{

					welcomelen = E.screencols;

				}

				int padding = (E.screencols - welcomelen) / 2;

				if (padding)
				{

					abAppend(ab, "~", 1);

					padding--;

				}

				while (padding--)
				{

					abAppend(ab, " ", 1);

				}

				abAppend(ab, welcome, welcomelen);

			}
			
			else
			{

				abAppend(ab, "~", 1);

			}
		} 
		else
		{
			int len = E.row[filerows].size;
			
			if(len > E.screencols)
			{
				len = E.screencols;
			}
			
			abAppend(ab, E.row[filerows].chars,len);
				
		}

        abAppend(ab, "\x1b[K", 3);

        if (y < E.screenrows - 1)
        {

            abAppend(ab, "\r\n", 2);

        }

    }

}

void editorRefreshScreen()
{
	editorScroll();
	
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);

    abAppend(&ab, "\x1b[H", 3);

    editordrawRows(&ab);

    char buf[32];

    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);

    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);

    abFree(&ab);

}

// input

void editorMoveCursor(int key)
{

    switch (key)
    {

        case ARROW_UP:
            if (E.cy != 0)
            {
                E.cy--;
            }
            break;

        case ARROW_LEFT:
            if (E.cx != 0)
            {
                E.cx--;
            }
            break;

        case ARROW_DOWN:
            if (E.cy < E.numrows)
            {
                E.cy++;
            }
            break;

        case ARROW_RIGHT:
            if (E.cx != E.screencols - 1)
            {
                E.cx++;
            }
            break;

    }

}

void editorProcessKeyPress()
{

    int c = editorReadKey();

    switch (c)
    {

        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);

            write(STDOUT_FILENO, "\x1b[H", 3);

            exit(0);

            break;
			
		case HOME_KEY:
			E.cx=0;
			
			break;
		
		case END_KEY:
			E.cx = E.screencols - 1;
			
			break;
		
	
        case PAGE_UP:
        case PAGE_DOWN:
        {

            int times = E.screenrows;

            while (times--)
            {
                editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }

        }
            break;

        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:

            editorMoveCursor(c);

            break;

    }

}

void initEditor()
{

    E.cx = 0;

    E.cy = 0;
	
	E.rowoff=0;
	
	E.numrows=0;
	
	E.row=NULL;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    {

        die("getWindowSize");

    }

}

int main(int argc, char *argv[])
{

    enableRawMode();

    initEditor();
	
	if(argc >= 2)
	{
		editorOpen(argv[1]);
	}

    while (1)
    {

        editorRefreshScreen();

        editorProcessKeyPress();

    }

    return 0;

}