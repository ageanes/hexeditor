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
        //LOG_MSG("key left");
      }
      break;
    case KEY_RIGHT :
      {
        //LOG_MSG("key right");
      }
      break;
    case KEY_DOWN :
      {
        editor_line_down_main();
        //LOG_MSG("Lines: first: %d, current: %d, last: %d", g_editor.screen.firstline_number, g_editor.screen.curline_number, g_editor.screen.lastline_number);
      }
      break;
    case KEY_UP :
      {
        editor_line_up_main();
        //LOG_MSG("Lines: first: %d, current: %d, last: %d", g_editor.screen.firstline_number, g_editor.screen.curline_number, g_editor.screen.lastline_number);
      }
      break;
  }
  return 0;
}

int insert_mode_exit() {
  return 0;
}
