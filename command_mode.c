#include "command_mode.h"

#include "editor.h"

#include <ncurses.h>

int cmd_mode_enter() {
  return 0;
}

int cmd_mode_place_cursor() {
  return 0;
}

int cmd_mode_new_char(int c) {
  switch(c) {
    case 'q' :
      return -1;
    case 'i' :
      editor_switch_mode(MODE_INSERT);
      break;
  }
  return 0;
}

int cmd_mode_exit() {
  return 0;
}
