#include "command_mode.h"

#include "editor.h"
#include "logger.h"

#include <ncurses.h>

int cmd_mode_enter() {
  editor_set_focus(g_windows.mainwnd);
  return 0;
}

int cmd_mode_place_cursor() {
  int by, y, bx, x;
  getyx(g_windows.mainwnd, y, x);
  getbegyx(g_windows.mainwnd, by, bx);
  move(by+y, bx+x);
  return 0;
}

int cmd_mode_new_char(int c) {
  switch(c) {
    case 'q' :
      return -1;
    case 'i' :
      editor_switch_mode(MODE_INSERT);
      break;
    case KEY_LEFT :
    case 'h':
      {
        int y, x, my, mx;
        getyx(g_windows.mainwnd, y, x);
        getmaxyx(g_windows.mainwnd, my, mx);
        if(x > 0) {
          wmove(g_windows.mainwnd, y, x-1);
        }
        LOG_MSG("key %d, moved main cursor to %d,%d\n", c, x-1, y);
      }
      break;
    case KEY_RIGHT :
    case 'l':
      {
        int y, x, my, mx;
        getyx(g_windows.mainwnd, y, x);
        getmaxyx(g_windows.mainwnd, my, mx);
        if(x < mx) {
          wmove(g_windows.mainwnd, y, x+1);
        }
        LOG_MSG("key %d, moved main cursor to %d,%d\n", c, x+1, y);
      }
      break;
    case KEY_DOWN :
    case 'j':
      {
        int y, x, my, mx;
        getyx(g_windows.mainwnd, y, x);
        getmaxyx(g_windows.mainwnd, my, mx);
        if(wmove(g_windows.mainwnd, y+1, x) == ERR) {
          editor_goto_line(editor_get_current_line() + 1);
        }
        LOG_MSG("key %d, moved main cursor to %d,%d\n", c, x, y+1);
      }
      break;
    case KEY_UP :
    case 'k':
      {
        int y, x, my, mx;
        getyx(g_windows.mainwnd, y, x);
        getmaxyx(g_windows.mainwnd, my, mx);
        if(wmove(g_windows.mainwnd, y-1, x) == ERR) {
          editor_goto_line(editor_get_current_line() - 1);
        }
        LOG_MSG("key %d, moved main cursor to %d,%d\n", c, x, y-1);
      }
      break;
  }
  return 0;
}

int cmd_mode_exit() {
  return 0;
}
