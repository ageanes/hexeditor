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
#include "editor.h"
#include "logger.h"

const char *g_progname;

// prototypes

void usage();

int main(int argc, char *argv[]) {
  g_progname = argv[0];

  if(argc < 2) {
    usage();
    return -1;
  }

  if(!setup_logger("log.txt")) {
    fprintf(stderr, "Could not set up logging facilities\n");
  }

  if(!setup_curses()) {
    fprintf(stderr, "Could not set up terminal\n");
    return -1;
  }

  if(!setup_windows()) {
    cleanup_curses();
    fprintf(stderr, "Could not set up terminal windows\n");
    return -1;
  }

  if(!open_file(argv[1])) {
    cleanup_windows();
    cleanup_curses();
    fprintf(stderr, "Could not set up terminal windows\n");
    return -1;
  }
  
  if(!setup_editor()) {
    cleanup_file();
    cleanup_windows();
    cleanup_curses();
    fprintf(stderr, "Could not set up the editor\n");
    return -1;
  }

  g_editor.mode = MODE_COMMAND - 1;
  editor_switch_mode(MODE_COMMAND);

  /*
  waddstr(g_windows.mainwnd, "Press 'q' in command mode to quit.  "
      "For command mode press escape.\n\nPress any key to continue, or 'q' to quit.");
  editor_refresh_windows();

  switch(getch()) {
    case 'q' :
    case 'Q' :
      goto cleanup_all;
      break;
  }
  */

  editor_redraw_main_window_full();

  editor_refresh_windows();

  while(1) { // command loop
    bool exit_loop = false;
    int c = getch(), r = 0;

    if(c == KEY_RESIZE) { // resize event
      if(editor_resize_windows() < 0) {
        fprintf(stderr, "Error during window resize\n");
        break;
      }
    }

    r = g_modes[g_editor.mode].new_char(c);

    editor_refresh_windows();

    if(r < 0) {
      break;
    }
  }

cleanup_all:
  cleanup_editor();
cleanup_file:
  cleanup_file();
cleanup_windows:
  cleanup_windows();
cleanup_curses:
  cleanup_curses();
cleanup_logger:
  cleanup_logger();
  return 0;
}

void usage() {
  fprintf(
    stderr, 
    "Usage: %s <file>\n"
      "  <file> the file you wish to view/edit\n\n"
      "Press 'q' in command mode to quit.\n"
      "Press 'i' in command mode to go to insert mode\n"
      "Press Esc in insert mode to go to command mode\n",
    g_progname
  );
}

