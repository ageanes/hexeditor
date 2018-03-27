#include "insert_mode.h"

#include "editor.h"

int insert_mode_enter() {
  return 0;
}

int insert_mode_place_cursor() {
  //g_editor_status.screen_curline = 0;
  return 0;
}

int insert_mode_new_char(int c) {
  switch(c) {
    case 27 : // escape
      editor_switch_mode(MODE_COMMAND);
      break;
    case KEY_LEFT :
      break;
    case KEY_RIGHT :
      break;
    case KEY_DOWN :
      break;
    case KEY_UP :
      break;
  }
  return 0;
}

int insert_mode_exit() {
  return 0;
}
