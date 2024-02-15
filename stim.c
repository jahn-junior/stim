/*** includes ***/

#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)
#define CLEAR_SCREEN() write(STDOUT_FILENO, "\x1b[2J", 4);
#define ABUF_INIT {NULL, 0}

/*** data ***/

struct abuf
{
  char* b;
  int len;
};

struct config
{
  int editor_rows;
  int editor_cols;
  struct termios orig_termios;
};

struct config conf;

/*** terminal ***/

void die(const char *s)
{
  CLEAR_SCREEN();
  perror(s);
  exit(1);
}

void disableRawMode()
{
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &conf.orig_termios) == -1)
  {
    die("tcsetattr failure");
  }
}

void enableRawMode()
{
  if (tcgetattr(STDIN_FILENO, &conf.orig_termios) == -1)
  {
    die("tcgetattr failure");
  }
  
  atexit(disableRawMode);

  struct termios raw = conf.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
  {
    die("tcsetattr failure");
  }
}

int getWindowSize(int* rows, int* cols)
{
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
  {
    return -1;
  }
  else
  {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** append buffer ***/

void append(struct abuf *ab, const char* s, int len)
{
  char* new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

/*** output ***/

void refreshScreen()
{
  struct abuf ab = ABUF_INIT;
  append(&ab, "\x1b[2J", 4);
  
  for (int y = 0; y < conf.editor_rows; y++)
  {
    char rowNum[16];
    snprintf(rowNum, 4, "%2.0f~", (float) y);
    append(&ab, rowNum, 4);
    if (y < conf.editor_rows - 1)
    {
      append(&ab, "\r\n", 2);
    }
  }

  append(&ab, "\x1b[H", 3);
  write(STDOUT_FILENO, ab.b, ab.len);
  free(ab.b);
}

/*** input ***/

char readInput()
{
  char c = '\0';
  
  if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
  {
    die("read failure");
  }

  return c;
}

void processInput(char c)
{
  switch(c)
  {
    case CTRL_KEY('x'):
      CLEAR_SCREEN()
      exit(0);
      break;
  }
}

/*** init ***/

int main()
{
  enableRawMode();
  if (getWindowSize(&conf.editor_rows, &conf.editor_cols) == -1)
  {
    die("getWindowSize failure");
  }
  refreshScreen();

  char c;
  while (1)
  {
    c = readInput();
    processInput(c);
  }
  
  return 0;
}
