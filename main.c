#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "main.h"

const char *g_progname;

struct {
  const char *filename;
  FILE *fp;       // file pointer
  int fd;         // file descriptor
  void *mm;       // mmapped area
  long mm_offset; // offset into file that is mmaped
  long mm_len;    // length of memory map
} g_curfile = 
  { 
    NULL,
    NULL,
    -1,
    NULL,
    0,
    0
  };

WINDOW *g_mainwnd = NULL;
WINDOW *g_inputwnd = NULL;
WINDOW *g_statuswnd = NULL;

struct {
  int mode;
} g_status = { 0 };

// prototypes

void usage();

int setup_curses();
int setup_windows();
int open_file(const char *filename);
void cleanup_file();
void cleanup_windows();
void cleanup_curses();

int main(int argc, char *argv[]) {
  g_progname = argv[0];

  if(argc < 2) {
    usage();
    return -1;
  }

  if(!setup_curses()) {
    fprintf(stderr, "Could not set up terminal\n");
    return -1;
  }

  if(!setup_windows()) {
    fprintf(stderr, "Could not set up terminal windows\n");
    return -1;
  }

  if(!open_file(argv[1])) {
    return -1;
  }

  while(1) { // command loop
    bool exit_loop = false;
    int r = getch();
    char buf[64];

    switch(r) {
      case 27: // escape key
        exit_loop = true;
        break;
      case 410: // resize
        {
          wresize(g_mainwnd, LINES - 2, COLS);
          mvwin(g_mainwnd, 0, 0);

          wresize(g_statuswnd, 1, COLS);
          mvwin(g_statuswnd, LINES - 2, 0);

          wresize(g_inputwnd, 1, COLS);
          mvwin(g_inputwnd, LINES - 1, 0);
        }
        break;
#if 0
      case KEY_LEFT:
        //mvwin(mywin, getbegy(mywin), getbegx(mywin) - 1);
        break;
      case KEY_RIGHT:
        //mvwin(mywin, getbegy(mywin), getbegx(mywin) + 1);
        break;
      case KEY_UP :
        //mvwin(mywin, getbegy(mywin) - 1, getbegx(mywin));
        break;
      case KEY_DOWN:
        //mvwin(mywin, getbegy(mywin) + 1, getbegx(mywin));
        break;
#endif // 0
      default:
        werase(stdscr);
        werase(g_mainwnd);
        werase(g_statuswnd);
        werase(g_inputwnd);

        wprintw(g_mainwnd, "Key pressed: %d %#x %#o\n", r, r, r);
        wprintw(g_mainwnd, "lines: %d, cols: %d\n", LINES, COLS);
        wprintw(g_mainwnd, "cur x y: %d %d\nmax x y: %d %d\n", getcurx(g_mainwnd), getcury(g_mainwnd), getmaxx(g_mainwnd), getmaxy(g_mainwnd));

        if(isprint(r)) {
          waddstr(g_statuswnd, "printable");
          waddch(g_inputwnd, r);
        } else {
          waddstr(g_statuswnd, "non-printable");
        }
    }

    /*
    wborder(mywin, 
        ACS_VLINE | COLOR_PAIR(1), 
        ACS_VLINE | COLOR_PAIR(1), 
        ACS_HLINE | COLOR_PAIR(1), 
        ACS_HLINE | COLOR_PAIR(1),
        ACS_ULCORNER | COLOR_PAIR(1),
        ACS_URCORNER | COLOR_PAIR(1),
        ACS_LLCORNER | COLOR_PAIR(1),
        ACS_LRCORNER | COLOR_PAIR(1));
        */
    wnoutrefresh(stdscr);
    wnoutrefresh(g_mainwnd);
    wnoutrefresh(g_statuswnd);
    wnoutrefresh(g_inputwnd);
    doupdate();
    
    if(exit_loop) {
      break;
    }

  }

  //delwin(mywin);
  cleanup_file();
  cleanup_windows();
  cleanup_curses();
  return 0;
}

int setup_curses() {
  WINDOW *r = initscr();
  if(!r) {
    return false;
  }

  if(has_colors()) {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    //init_pair(1, COLOR_CYAN, COLOR_BLACK);
    //init_pair(2, COLOR_BLACK, COLOR_WHITE);
  }

  keypad(stdscr, true); // make arrow keys work right
  noecho();
  //cbreak(); // let Ctrl+C send a SIGINTR
  cbreak(); // pass 
  nonl();

  // set the timeout for escape key presses to 10 msec
  ESCDELAY=10;

  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
  //curs_set(0); // hide the cursor by default

  //wbkgdset(stdscr, COLOR_PAIR(2));
  werase(stdscr);
  wrefresh(stdscr);

  return true;
}

int setup_windows() {
  g_mainwnd = newwin(LINES - 2, COLS, 0, 0);
  if(g_mainwnd) {
    //wkbgdset(g_mainwnd, 0);
    wbkgdset(g_mainwnd, 0);
    waddstr(g_mainwnd, "Main window");
    wnoutrefresh(g_mainwnd);
  }
  g_statuswnd = newwin(1, COLS, LINES - 2, 0);
  if(g_statuswnd) {
    wattrset(g_statuswnd, COLOR_PAIR(1));
    waddstr(g_statuswnd, "Status window");
    wnoutrefresh(g_statuswnd);
  }
  g_inputwnd = newwin(1, COLS, LINES - 1, 0);
  if(g_inputwnd) {
    //wattrset(g_statuswnd, COLOR_PAIR(1));
    waddstr(g_inputwnd, "Input window");
    wnoutrefresh(g_inputwnd);
  }

  doupdate();

  return g_mainwnd && g_inputwnd && g_statuswnd;
}

void cleanup_curses() {
  endwin();
}

void usage() {
  fprintf
  (
    stderr, 
    "Usage: %s <file>\n"
      "  <file> the file you wish to view/edit\n",
    g_progname);
}

int open_file(const char *filename) {
  int fd;
  FILE *fp;
  fd = open(filename, O_RDWR);
  if(fd < 0) {
    fprintf(stderr, "Could not open %s\n", filename);
    return false;
  }

  g_curfile.fd = fd;

  fd = dup(fd); // duplicate the descriptor so that we can open a FILE*
  if(fd < 0) {
    close(g_curfile.fd);
    return false;
  }

  fp = fdopen(fd, "r+");
  if(!fp) {
    close(fd);
    close(g_curfile.fd);
    return false;
  }

  g_curfile.fp = fp;
  g_curfile.filename = filename;

  return true;
}

void cleanup_file() {
  if(g_curfile.mm) {
    munmap(g_curfile.mm, g_curfile.mm_len);
    g_curfile.mm = NULL;
  }

  if(g_curfile.fp) {
    fclose(g_curfile.fp);
    g_curfile.fp = NULL;
  }

  if(g_curfile.fd >= 0) {
    close(g_curfile.fd);
    g_curfile.fd = -1;
  }

  g_curfile.filename = NULL;
}

void cleanup_windows() {
  if(g_mainwnd) delwin(g_mainwnd);
  if(g_inputwnd) delwin(g_inputwnd);
  if(g_statuswnd) delwin(g_statuswnd);
}
