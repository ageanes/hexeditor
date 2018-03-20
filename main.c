#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "main.h"

const char *g_progname;

int setup_curses();
void cleanup_curses();

int main(int argc, char *argv[]) {
  g_progname = argv[0];

  setup_curses();

  ESCDELAY=100;

  WINDOW *mywin = newwin(LINES / 2, COLS / 2, 0, 0);
  if(!mywin) {
    printw("No window");
    sleep(1);
    exit(-2);
  }
  wprintw(mywin, "Has colors? %d.  ESCDELAY: %d\n", has_colors(), ESCDELAY);
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
  curs_set(0);

  wbkgdset(stdscr, COLOR_PAIR(2));
  werase(mywin);
  wrefresh(stdscr);

  while(1) {
    bool exit_loop = false;
    int r = getch();
    char buf[64];

    switch(r) {
      case 27: // escape key
        exit_loop = true;
        break;
      case 410: // resize
        {
          werase(mywin);
          wnoutrefresh(mywin);
          
          int cx, cy, mx, my;
          getbegyx(mywin, cy, cx);
          getmaxyx(mywin, my, mx);
          if(cx+mx >= COLS) {
            cx = COLS - mx;
          } else if(cx < 0) {
            cx = 0;
          }
          if(cy+my >= LINES) {
            cy = LINES - my;
          } else if(cy < 0) {
            cy = 0;
          }
          if(mx-cx <= COLS && my-cy <= LINES) {
            mvwin(mywin, cy, cx);
          }
          wmove(mywin, 1, 1);
          wprintw(mywin, "Moving to %d,%d", cx, cy);
          wmove(mywin, getcury(mywin) + 1, 1);
          wprintw(mywin, "Max window size %d,%d\n", mx, my);
        }
        break;
      case KEY_LEFT:
        mvwin(mywin, getbegy(mywin), getbegx(mywin) - 1);
        break;
      case KEY_RIGHT:
        mvwin(mywin, getbegy(mywin), getbegx(mywin) + 1);
        break;
      case KEY_UP :
        mvwin(mywin, getbegy(mywin) - 1, getbegx(mywin));
        break;
      case KEY_DOWN:
        mvwin(mywin, getbegy(mywin) + 1, getbegx(mywin));
        break;
      default:
        {
          int l;
          werase(mywin);

          if(r == KEY_MOUSE) {
            MEVENT m;
            getmouse(&m);
            snprintf(buf, 64, "Mouse %d, (%d,%d,%d)", m.id, m.x, m.y, m.z);
          } else {
            snprintf(buf, 64, "Key %d (%#x, %#o), term size %dx%d", r, r, r, COLS, LINES);
          }
          l = strlen(buf);

          int y, x;
          wmove(mywin, 1, 1);

          /*
          waddch(mywin, ACS_ULCORNER);
          for(int i = 0; i < l; ++i) {
            waddch(mywin, ACS_HLINE);
          }
          waddch(mywin, ACS_URCORNER);
          waddch(mywin, '\n');
          waddch(mywin, ACS_VLINE);
          */
          waddstr(mywin, buf);
          getyx(mywin, y, x);
          wmove(mywin, y+1, 1);
          /*
          waddch(mywin, ACS_VLINE);
          waddch(mywin, '\n');
          waddch(mywin, ACS_VLINE);
          */

          //int last = ACS_VLINE;
          if(isprint(r)) {
            waddstr(mywin, "Character: ");
            waddch(mywin, r | A_BOLD | A_UNDERLINE);
          } else {
            waddstr(mywin, "Non-printable key");
          }

          /*
          for(int i = 0; i < l - 1; ++i) {
            waddch(mywin, ACS_HLINE);
          }
          waddch(mywin, last);
          waddch(mywin, '\n');
          waddch(mywin, ACS_LLCORNER);
          for(int i = 0; i < l; ++i) {
            waddch(mywin, ACS_HLINE);
          }
          waddch(mywin, ACS_LRCORNER);
          */
          //printw("nuts");
        }
    }

    wborder(mywin, 
        ACS_VLINE | COLOR_PAIR(1), 
        ACS_VLINE | COLOR_PAIR(1), 
        ACS_HLINE | COLOR_PAIR(1), 
        ACS_HLINE | COLOR_PAIR(1),
        ACS_ULCORNER | COLOR_PAIR(1),
        ACS_URCORNER | COLOR_PAIR(1),
        ACS_LLCORNER | COLOR_PAIR(1),
        ACS_LRCORNER | COLOR_PAIR(1));
    werase(stdscr);
    wnoutrefresh(stdscr);
    wnoutrefresh(mywin);
    doupdate();
    
    if(exit_loop) {
      break;
    }

  }

  delwin(mywin);
  cleanup_curses();
  return 0;
}

int setup_curses() {
  WINDOW *r = initscr();
  if(!r) {
    exit(-1);
  }

  if(has_colors()) {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
  }
  keypad(stdscr, true); // make arrow keys work right
  noecho();
  //cbreak(); // let Ctrl+C send a SIGINTR
  raw(); // pass 
  nonl();
  return 0;
}

void cleanup_curses() {
  endwin();
}
