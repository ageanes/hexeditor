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
        editor_char_left_main();
        LOG_MSG("key left");
      }
      break;
    case KEY_RIGHT :
    case 'l':
      {
        editor_char_right_main();
        LOG_MSG("key right");
      }
      break;
    case KEY_DOWN :
    case 'j':
      {
        editor_line_down_main();
        LOG_MSG("Lines: first: %d, current: %d, last: %d", g_editor.screen.firstline_number, g_editor.screen.curline_number, g_editor.screen.lastline_number);
      }
      break;
    case KEY_UP :
    case 'k':
      {
        editor_line_up_main();
        LOG_MSG("Lines: first: %d, current: %d, last: %d", g_editor.screen.firstline_number, g_editor.screen.curline_number, g_editor.screen.lastline_number);
      }
      break;
  }
  return 0;
}

int cmd_mode_exit() {
  return 0;
}
