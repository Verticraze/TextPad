// includes

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE


#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define VCPAD_TAB_STOP 8
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
	
	int rsize;
	
	char *chars;

	char *render;
	
}erow;

struct editorConfig
{
    int cx, cy;
	
	int rx;
	
	int rowoff;
	
	int coloff;

    int screenrows;

    int screencols;

	int numrows;
	
	erow *row;
	
	char *filename;
	
	char statusMessage[80];
	
	time_t statusMessageTime;

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

int editorRowCxToRx(erow *row, int cx)
{
	int rx=0;
	int j;
	for(j = 0 ; j < cx ; j++)
	{
		if(row->chars[j]=='\t')
		{
			rx += (VCPAD_TAB_STOP - 1) - (rx % VCPAD_TAB_STOP);
		}
		rx++;
	}
	
	return rx;
}


void editorUpdateRow(erow *row)
{
	int tabs = 0;
	
	int j;


	for(j = 0 ; j < row -> size ; j++)
	{
		if(row -> chars[j] == '\t')
		{
			tabs++;
		}
	}

	
	free(row->render);

	row->render=malloc(row->size + tabs * (VCPAD_TAB_STOP - 1) + 1);
	
	int idx = 0;
	
	for(j = 0 ; j < row -> size ; j++)
	{
		if(row -> chars[j] == '\t')
		{
			row -> render[idx++] = ' ';
			
			while(idx % VCPAD_TAB_STOP != 0)
			{
					row->render[idx++]=' ';
			}
		}
		else
		{
			row -> render[idx++] = row -> chars[j];
		}
		
	}
	
	row -> render[idx] = '\0';

	row -> rsize = idx;
}

void editorAppendRow(char *s, size_t len)
{
	E.row = realloc(E.row,sizeof(erow)*(E.numrows+1));
	
	int pos = E.numrows;
	
	E.row[pos].size = len;
	
	E.row[pos].chars = malloc(len+1);
	
	memcpy(E.row[pos].chars,s,len);
	
	E.row[pos].chars[len] = '\0';
	
	E.row[pos].rsize = 0;
	
	E.row[pos].render = NULL;
	
	editorUpdateRow(&E.row[pos]);
	
	E.numrows++;
}

void editorOpen(char *filename)
{
	free(E.filename);
	
	E.filename=strdup(filename);
	
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
	E.rx = 0;
	
	if(E.cy < E.numrows)
	{
		E.rx = editorRowCxToRx(&E.row[E.cy],E.cx);
	}
	
	if(E.cy < E.rowoff)
	{
		E.rowoff=E.cy;
	}
	
	if(E.cy >= E.rowoff + E.screenrows)
	{
		E.rowoff = E.cy - E.screenrows + 1;
	}
	
	if(E.rx < E.coloff)
	{
		E.coloff = E.rx;
	}
		
	if(E.rx >= E.coloff + E.screencols)
	{
		E.coloff = E.rx - E.screencols + 1;
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
			int len = E.row[filerows].rsize - E.coloff;
			
			if(len<0)
			{
				len=0;
			}
			
			if(len > E.screencols)
			{
				len = E.screencols;
			}
			
			abAppend(ab, &E.row[filerows].render[E.coloff],len);
				
		}

        abAppend(ab, "\x1b[K", 3);

		abAppend(ab, "\r\n", 2);

    }

}

void editorDrawStatusBar(struct abuf *ab)
{
	abAppend(ab,"\x1b[7m",4);

	char status[80], rowStatus[80];
	
	int len = snprintf(status,sizeof(status),"%.20s - %d lines ", E.filename ? E.filename : " No Name ", E.numrows);

	int rowlen = snprintf(rowStatus, sizeof(rowStatus),"%d,%d", E.cy + 1,E.numrows);
	
	if(len > E.screencols)
	{
		len = E.screencols;
	}
	
	abAppend(ab,status,len);
	
	while(len < E.screencols)
	{	
		if(E.screencols - len == rowlen)
		{
			abAppend(ab,rowStatus,rowlen);
			
			break;
		}
		else
		{
			abAppend(ab," ",1);
		
			len++;
		}
	}
	
	abAppend(ab,"\x1b[m",3);
	
	abAppend(ab,"\r\n",2);
}

void editorDrawMessageBar(struct abuf *ab)
{
	abAppend(ab,"\x1b[K",3);
	
	int messageLength = strlen(E.statusMessage);
	
	if(messageLength > E.screencols)
	{
		messageLength = E.screencols;
	}
	
	if(messageLength&&time(NULL) - E.statusMessageTime < 5)
	{
		abAppend(ab,E.statusMessage,messageLength);
	}
}

void editorRefreshScreen()
{
	editorScroll();
	
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);

    abAppend(&ab, "\x1b[H", 3);

    editordrawRows(&ab);
	
	editorDrawStatusBar(&ab);
	
	editorDrawMessageBar(&ab);

    char buf[32];

    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);

    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);

    abFree(&ab);

}

void editorSetStatusMessage(const char* fmt,...)
{
	va_list ap;
	
	va_start(ap, fmt);
	
	vsnprintf(E.statusMessage,sizeof(E.statusMessage),fmt,ap);
	
	va_end(ap);
	
	E.statusMessageTime=time(NULL);
}

// input

void editorMoveCursor(int key)
{
	erow *row = (E.cy > E.numrows) ? NULL : &E.row[E.cy];
   
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
			else if(E.cy > 0)
			{
				E.cy--;
				E.cx = E.row[E.cy].size;
			}
            break;

        case ARROW_DOWN:
            if (E.cy < E.numrows)
            {
                E.cy++;
            }
            break;

        case ARROW_RIGHT:
	
			if(row && E.cx< row -> size)
			{
				E.cx++;
			}
			
			else if(row && E.cx == row -> size)
			{
				E.cy++;
				E.cx=0;
			}
            
			break;

    }
	
	row = E.cy >= E.numrows ? NULL : &E.row[E.cy];
	
	int rowlen = row ? row -> size : 0;
	
	if(E.cx > rowlen)
	{
		E.cx=rowlen;
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
			if(E.cy < E.numrows)
			{
				E.cx = E.row[E.cy].size;
			}
			
			break;
		
	
        case PAGE_UP:
        case PAGE_DOWN:
        {
			if(c == PAGE_UP)
			{
				E.cy = E.rowoff;
			}
			else if(c == PAGE_DOWN)
			{
				E.cy = E.rowoff + E.screenrows - 1;
			}
			if(E.cy > E.numrows)
			{
				E.cy = E.numrows;
			}
			
			
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
	
	E.rx = 0;
	
	E.rowoff = 0;
	
	E.coloff = 0;
	
	E.numrows = 0;
	
	E.row = NULL;
	
	E.filename = NULL;
	
	E.statusMessage[0] = '\0';
	
	E.statusMessageTime = 0;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    {

        die("getWindowSize");

    }
	
	E.screenrows -= 2;

}

int main(int argc, char *argv[])
{

    enableRawMode();

    initEditor();
	
	if(argc >= 2)
	{
		editorOpen(argv[1]);
	}

	editorSetStatusMessage("HELLLLLLLLLLLLLLLLP");

    while (1)
    {

        editorRefreshScreen();

        editorProcessKeyPress();

    }

    return 0;

}