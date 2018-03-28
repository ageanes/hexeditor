#include "insert_mode.h"

#include "editor.h"
#include "logger.h"

int insert_mode_enter() {
  editor_set_focus(g_windows.mainwnd);
  return 0;
}

int insert_mode_place_cursor() {
  //g_editor_status.screen_curline = 0;
  int by, y, bx, x;
  getyx(g_windows.mainwnd, y, x);
  getbegyx(g_windows.mainwnd, by, bx);
  move(by+y, bx+x);
  return 0;
}

WINDOW *insert_mode_get_focus() {
  return g_windows.mainwnd;
}

int insert_mode_new_char(int c) {
  switch(c) {
    case 27 : // escape
      editor_switch_mode(MODE_COMMAND);
      break;
    case KEY_LEFT :
      {
        int y, x, my, mx;
        getyx(g_windows.mainwnd, y, x);
        getmaxyx(g_windows.mainwnd, my, mx);
        if(x > 0) {
          wmove(g_windows.mainwnd, y, x-1);
        }
        LOG_MSG("Moved main cursor to %d,%d\n", x-1, y);
      }
      break;
    case KEY_RIGHT :
      {
        int y, x, my, mx;
        getyx(g_windows.mainwnd, y, x);
        getmaxyx(g_windows.mainwnd, my, mx);
        if(x < mx) {
          wmove(g_windows.mainwnd, y, x+1);
        }
        LOG_MSG("Moved main cursor to %d,%d\n", x+1, y);
      }
      break;
    case KEY_DOWN :
      {
        int y, x, my, mx;
        getyx(g_windows.mainwnd, y, x);
        getmaxyx(g_windows.mainwnd, my, mx);
        if(y < my) {
          wmove(g_windows.mainwnd, y+1, x);
        } else {
          editor_goto_line(editor_get_current_line() + 1);
        }
        LOG_MSG("Moved main cursor to %d,%d\n", x, y+1);
      }
      break;
    case KEY_UP :
      {
        int y, x, my, mx;
        getyx(g_windows.mainwnd, y, x);
        getmaxyx(g_windows.mainwnd, my, mx);
        if(y > 0) {
          wmove(g_windows.mainwnd, y-1, x);
        } else {
          editor_goto_line(editor_get_current_line() - 1);
        }
        LOG_MSG("Moved main cursor to %d,%d\n", x, y-1);
      }
      break;
  }
  return 0;
}

int insert_mode_exit() {
  return 0;
}
