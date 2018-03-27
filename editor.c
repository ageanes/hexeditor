#include "editor.h"

#include "command_mode.h"
#include "insert_mode.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h> 

#include <ctype.h>

editor_window g_windows = { 
  .mainwnd = NULL, 
  .statuswnd = NULL, 
  .inputwnd = NULL 
};

mode_ops g_modes[] = {
  {
    "command",
    cmd_mode_enter,
    cmd_mode_new_char,
    cmd_mode_place_cursor,
    cmd_mode_exit
  },
  {
    "insert",
    insert_mode_enter,
    insert_mode_new_char,
    insert_mode_place_cursor,
    insert_mode_exit
  },
  {
    NULL,
    NULL,
    NULL,
    NULL
  }
};

editor_status g_editor_status = { 
  .mode = MODE_COMMAND,
  .buf = {
    .lines = NULL,
    .linebuf = NULL,
    .linebuf_len = 0
  },
  .firstline = NULL,
  .firstline_number = 0,
  .curline = NULL,
};

file_info g_curfile = { 
  .filename = NULL, 
  .fp = NULL, 
  .fd = -1, 
  .mm = NULL, 
  .mm_offset = 0, 
  .mm_len = 0 
};

// functions

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
  g_windows.mainwnd = newwin(LINES - 2, COLS, 0, 0);
  if(g_windows.mainwnd) {
    //wkbgdset(g_windows.mainwnd, 0);
    wbkgdset(g_windows.mainwnd, 0);
    waddstr(g_windows.mainwnd, "Main window");
    wnoutrefresh(g_windows.mainwnd);
  }
  g_windows.statuswnd = newwin(1, COLS, LINES - 2, 0);
  if(g_windows.statuswnd) {
    wattrset(g_windows.statuswnd, COLOR_PAIR(1));
    waddstr(g_windows.statuswnd, "Status window");
    wnoutrefresh(g_windows.statuswnd);
  }
  g_windows.inputwnd = newwin(1, COLS, LINES - 1, 0);
  if(g_windows.inputwnd) {
    //wattrset(g_windows.statuswnd, COLOR_PAIR(1));
    waddstr(g_windows.inputwnd, "Input window");
    wnoutrefresh(g_windows.inputwnd);
  }

  if(g_windows.mainwnd && g_windows.statuswnd && g_windows.inputwnd) {
    werase(stdscr);
    werase(g_windows.mainwnd);
    werase(g_windows.statuswnd);
    werase(g_windows.inputwnd);
  }

  doupdate();

  return g_windows.mainwnd && g_windows.inputwnd && g_windows.statuswnd;
}

void cleanup_curses() {
  endwin();
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

  struct stat s;
  if(fstat(g_curfile.fd, &s) < 0) {
    close(fd);
    close(g_curfile.fd);
    return false;
  }

  char *mm = mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, g_curfile.fd, 0);
  if(!mm) {
    close(fd);
    close(g_curfile.fd);
    return false;
  }

  g_curfile.mm = mm;
  g_curfile.mm_len = s.st_size;
  g_curfile.mm_offset = 0;
  return true;
}

int setup_editor() {
  size_t bmark = 0, emark = 0;
  struct editor_line *last_line = NULL;

  while(bmark < g_curfile.mm_len) {
    while(emark < g_curfile.mm_len && g_curfile.mm[emark++] != '\n');
    gap_buffer *gb = gap_buffer_new(emark - bmark);
    if(gb) {
      struct editor_line *el = malloc(sizeof(struct editor_line));
      if(el) {
        el->gb = gb;
        el->next = NULL;
        el->prev = last_line;
        if(last_line) {
          last_line->next = el;
        } else {
          g_editor_status.buf.lines = el;

          g_editor_status.firstline = el;
          g_editor_status.curline = el;
          g_editor_status.firstline_number = 1;
          g_editor_status.screen_curline = 0;
        }
        last_line = el;

        for(size_t i = bmark; i < emark; ++i) {
          gap_buffer_addch(gb, g_curfile.mm[i]);
        }
        gap_buffer_setpos(gb, 0);
      } else {
        gap_buffer_delete(gb);
        gb = NULL;
      }
    }
    bmark = emark;
  }

  g_editor_status.buf.linebuf = malloc(COLS);
  if(g_editor_status.buf.linebuf) {
    g_editor_status.buf.linebuf_len = COLS;
  }
  return true;
}

void cleanup_editor() {
  struct editor_line *el = g_editor_status.buf.lines;
  while(el) {
    struct editor_line *next = el->next;
    if(el->gb) {
      gap_buffer_delete(el->gb);
      el->gb = NULL;
    }
    free(el);
    el = next;
  }

  g_editor_status.buf.lines = NULL;
  g_editor_status.firstline = NULL;
  g_editor_status.curline = NULL;
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
  if(g_windows.mainwnd) delwin(g_windows.mainwnd);
  if(g_windows.inputwnd) delwin(g_windows.inputwnd);
  if(g_windows.statuswnd) delwin(g_windows.statuswnd);
}

void clear_status_window() {
  werase(g_windows.statuswnd);
}

void set_status_window_text(const char *str) {
  clear_status_window();
  waddstr(g_windows.statuswnd, str);
}

void append_status_window_text(const char *str) {
  waddstr(g_windows.statuswnd, str);
}

void clear_input_window() {
  werase(g_windows.inputwnd);
}

void set_input_window_text(const char *str) {
  clear_input_window();
  waddstr(g_windows.inputwnd, str);
}

void append_input_window_text(const char *str) {
  waddstr(g_windows.inputwnd, str);
}

void append_input_window_char(char c) {
  waddch(g_windows.inputwnd, c);
}

int editor_switch_mode(int to_mode) {
  int r = 0;
  if(to_mode < MODE_COMMAND || to_mode >= NUMBER_MODES) {
    return -1;
  }

  if(to_mode == g_editor_status.mode) {
    return r;
  }

  if(g_editor_status.mode >= MODE_COMMAND && g_editor_status.mode < NUMBER_MODES) {
    if(g_modes[g_editor_status.mode].exit_mode() < 0) {
      r = -1;
    }
  }
  g_editor_status.mode = to_mode;
  if(g_modes[g_editor_status.mode].enter_mode() < 0) {
    r = -1;
  }

  clear_input_window();
  set_status_window_text(g_modes[g_editor_status.mode].mode_name);

  return r;
}

int editor_resize_windows() {
  wresize(g_windows.mainwnd, LINES - 2, COLS);
  mvwin(g_windows.mainwnd, 0, 0);

  wresize(g_windows.statuswnd, 1, COLS);
  mvwin(g_windows.statuswnd, LINES - 2, 0);

  wresize(g_windows.inputwnd, 1, COLS);
  mvwin(g_windows.inputwnd, LINES - 1, 0);

  if(g_editor_status.buf.linebuf) {
    char *new_linebuf = realloc(g_editor_status.buf.linebuf, COLS);
    if(new_linebuf) {
      g_editor_status.buf.linebuf = new_linebuf;
      g_editor_status.buf.linebuf_len = COLS;
    } else {
      return -1;
    }
  } else {
    char *new_linebuf = malloc(COLS);
    if(new_linebuf) {
      g_editor_status.buf.linebuf = new_linebuf;
      g_editor_status.buf.linebuf_len = COLS;
    } else {
      return -1;
    }
  }

  editor_refresh_main_window();

  return 0;
}

void editor_update_windows() {
  if(g_editor_status.mode >= MODE_COMMAND 
      && g_editor_status.mode < NUMBER_MODES
      && g_modes[g_editor_status.mode].place_cursor) {
    g_modes[g_editor_status.mode].place_cursor();
  }
  wnoutrefresh(stdscr);
  wnoutrefresh(g_windows.mainwnd);
  wnoutrefresh(g_windows.statuswnd);
  wnoutrefresh(g_windows.inputwnd);
  doupdate();
}

void editor_refresh_main_window() {
  int mx, my, cx, cy, cur_line = 0;

  werase(g_windows.mainwnd);

  if(!g_editor_status.buf.linebuf) {
    editor_update_windows();
    return;
  }

  getmaxyx(g_windows.mainwnd, my, mx);
  wmove(g_windows.mainwnd, 0, 0);

  struct editor_line *el = g_editor_status.firstline;
  char *linebuf = g_editor_status.buf.linebuf;
  size_t linebuf_len = g_editor_status.buf.linebuf_len;
  while(el && cur_line < my) {
    int n_copied = gap_buffer_copy(el->gb, linebuf, linebuf_len);
    for(int i = 0; i < n_copied; ++i) {
      if(isprint(linebuf[i])) {
        waddch(g_windows.mainwnd, linebuf[i]);
      } else {
        waddch(g_windows.mainwnd, ' ');
      }
    }
    wmove(g_windows.mainwnd, getcury(g_windows.mainwnd) + 1, 0);
    el = el->next;
    ++cur_line;
  }

  editor_update_windows();
}

void editor_line_down() {
}

void editor_line_up() {
}
