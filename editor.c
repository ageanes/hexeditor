#include "editor.h"

#include "command_mode.h"
#include "insert_mode.h"
#include "logger.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h> 
#include <stdlib.h> 
#include <limits.h>
#include <assert.h>

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
    cmd_mode_exit
  },
  {
    "insert",
    insert_mode_enter,
    insert_mode_new_char,
    insert_mode_exit
  },
  {
    NULL,
    NULL,
    NULL,
    NULL
  }
};

editor_status g_editor = { 
  .mode = MODE_COMMAND,
  .data = {
    .lines = NULL,
    .firstline = NULL,
    .lastline = NULL,
    .n_lines = 0
  },
  .screen = {
    .focus = NULL,
    .firstline = NULL,
    .firstline_number = 0,
    .lastline = NULL,
    .lastline_number = 0,
    .curline = NULL,
    .curline_number = 0,
    .linebuf = NULL,
    .linebuf_len = 0
  }
};

file_info g_curfile = { 
  .filename = NULL, 
  .fp = NULL, 
  .fd = -1, 
  .mm = NULL, 
  .mm_offset = 0, 
  .mm_len = 0 
};

int editor_line_list_len() {
  int n = 0;
  struct editor_line *el = g_editor.screen.firstline;
  while(el) {
    ++n;
    el = el->next;
  }
  return n;
}

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
  // main window, full screen height - 2, full screen width
  g_windows.mainwnd = newwin(LINES - 2, COLS, 0, 0);
  if(g_windows.mainwnd) {
    wbkgdset(g_windows.mainwnd, 0);
    wnoutrefresh(g_windows.mainwnd);
  }
  
  // status window, 1 character high, full width of screen, below main window
  g_windows.statuswnd = newwin(1, COLS, LINES - 2, 0);
  if(g_windows.statuswnd) {
    wattrset(g_windows.statuswnd, COLOR_PAIR(1));
    wnoutrefresh(g_windows.statuswnd);
  }
  
  // input window, 1 character high, full width of screen, below status window
  g_windows.inputwnd = newwin(1, COLS, LINES - 1, 0); 
  if(g_windows.inputwnd) {
    wnoutrefresh(g_windows.inputwnd);
  }

  if(!g_windows.mainwnd || !g_windows.statuswnd || !g_windows.inputwnd) {
    return false;
  }

  // update window sizes in editor structures
  editor_update_window_sizes();

  // clear the screens
  werase(stdscr);
  werase(g_windows.mainwnd);
  werase(g_windows.statuswnd);
  werase(g_windows.inputwnd);

  doupdate();

  return true;
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
  size_t n_lines = 0;

  // read through file data and separate into lines
  // each line in the file is assigned to one gap buffer
  while(bmark < g_curfile.mm_len) {

    // find the next instance of a newline
    while(emark < g_curfile.mm_len && g_curfile.mm[emark++] != '\n');

    // create a gap buffer for the current line
    gap_buffer *gb = gap_buffer_new(emark - bmark);
    if(!gb) {
      return false;
    }

    struct editor_line *el = malloc(sizeof(struct editor_line));
    if(!el) {
      gap_buffer_delete(gb);
      gb = NULL;
      return false;
    }

    el->gb = gb;
    el->next = NULL;
    el->prev = last_line;

    if(last_line) {
      last_line->next = el;
    } else {
      // set up the editor state if this is the first line
      g_editor.data.lines = el;
      g_editor.data.firstline = el;
    }

    // load data into the gap buffer, skip the newline at the end
    for(size_t i = bmark; i < emark - 1; ++i) {
      gap_buffer_addch(gb, g_curfile.mm[i]);
    }
    gap_buffer_setcursor(gb, 0);

    last_line = el;
    ++n_lines;
    bmark = emark;
  }
  g_editor.data.lastline = last_line;
  g_editor.data.n_lines = n_lines;

  // set up the line buffer for the screen
  assert(!g_editor.screen.linebuf);
  char *new_linebuf = malloc(g_windows.mainwnd_geom.w);
  if(!new_linebuf) {
    return false;
  }
  g_editor.screen.linebuf = new_linebuf;
  g_editor.screen.linebuf_len = g_windows.mainwnd_geom.w;

  g_editor.screen.focus = g_windows.mainwnd;

  editor_goto_line_scan(0);

  return true;
}

void cleanup_editor() {
  struct editor_line *el = g_editor.data.lines;
  char *linebuf = g_editor.screen.linebuf;

  g_editor.data.lines = NULL;
  g_editor.data.firstline = NULL;
  g_editor.data.lastline = NULL;
  g_editor.data.n_lines = 0;

  g_editor.screen.focus = NULL;
  g_editor.screen.firstline = NULL;
  g_editor.screen.firstline_number = 0;
  g_editor.screen.lastline = NULL;
  g_editor.screen.lastline_number = 0;
  g_editor.screen.curline = NULL;
  g_editor.screen.curline_number = 0;

  g_editor.screen.linebuf = NULL;
  g_editor.screen.linebuf_len = 0;

  while(el) {
    struct editor_line *next = el->next;
    if(el->gb) {
      gap_buffer_delete(el->gb);
      el->gb = NULL;
    }
    free(el);
    el = next;
  }

  if(linebuf) {
    free(linebuf);
  }

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

void editor_set_focus(WINDOW *window) {
  g_editor.screen.focus = window;
}

void editor_place_cursor() {
  if(!g_editor.screen.focus) {
    return;
  }
  int y, x, by, bx;
  getyx(g_editor.screen.focus, y, x);
  getbegyx(g_editor.screen.focus, by, bx);
  move(by+y, bx+x);
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

  if(to_mode == g_editor.mode) {
    return r;
  }

  if(g_editor.mode >= MODE_COMMAND && g_editor.mode < NUMBER_MODES) {
    if(g_modes[g_editor.mode].exit_mode() < 0) {
      r = -1;
    }
  }
  g_editor.mode = to_mode;
  if(g_modes[g_editor.mode].enter_mode() < 0) {
    r = -1;
  }

  clear_input_window();
  set_status_window_text(g_modes[g_editor.mode].mode_name);

  return r;
}

int screen_lines_in_buffer(struct editor_line *line) {
  int n = 0;
  if(!line || !line->gb || g_windows.mainwnd_geom.w == 0) {
    return n;
  }

  int line_len = line->gb->len;

  // round the line length up to the nearest multiple of the main window width
  // then divide by the 
  line_len += g_windows.mainwnd_geom.w - 1;
  n = line_len / g_windows.mainwnd_geom.w;

  return n;
}

void editor_update_window_sizes() {
  // windows are arranged:
  // ----------
  //
  //    main 
  //
  // --status--
  // --input---
  //
  // All windows are full screen width
  //
  wresize(g_windows.mainwnd, LINES - 2, COLS);
  mvwin(g_windows.mainwnd, 0, 0);
  getbegyx(g_windows.mainwnd, g_windows.mainwnd_geom.y, g_windows.mainwnd_geom.x);
  getmaxyx(g_windows.mainwnd, g_windows.mainwnd_geom.h, g_windows.mainwnd_geom.w);

  wresize(g_windows.statuswnd, 1, COLS);
  mvwin(g_windows.statuswnd, LINES - 2, 0);
  getbegyx(g_windows.statuswnd, g_windows.statuswnd_geom.y, g_windows.statuswnd_geom.x);
  getmaxyx(g_windows.statuswnd, g_windows.statuswnd_geom.h, g_windows.statuswnd_geom.w);

  wresize(g_windows.inputwnd, 1, COLS);
  mvwin(g_windows.inputwnd, LINES - 1, 0);
  getbegyx(g_windows.inputwnd, g_windows.inputwnd_geom.y, g_windows.inputwnd_geom.x);
  getmaxyx(g_windows.inputwnd, g_windows.inputwnd_geom.h, g_windows.inputwnd_geom.w);
}

int editor_resize_windows() {

  editor_update_window_sizes();

  char *new_linebuf = realloc(g_editor.screen.linebuf, g_windows.mainwnd_geom.w);
  if(g_windows.mainwnd_geom.w && !new_linebuf) { // check for 0-length buffer if width > 0
    return -1;
  }
  g_editor.screen.linebuf = new_linebuf;
  g_editor.screen.linebuf_len = g_windows.mainwnd_geom.w;

  // determine what lines are displayed on screen
  
  struct editor_line *el = g_editor.screen.firstline;
  int lineno = 0;

  // lastline is the last line displayed on screen, so we want to stop on that line
  int dest_line = g_windows.mainwnd_geom.h ? g_windows.mainwnd_geom.h - 1 : 0;
  while(el && el->next && lineno < g_windows.mainwnd_geom.h ) {
    el = el->next;
    ++lineno;
  }

  g_editor.screen.lastline = el;
  g_editor.screen.lastline_number = g_editor.screen.firstline_number + lineno;

  if(g_editor.screen.curline_number > g_editor.screen.lastline_number) {

    int curpos = g_editor.screen.curline_cursor;
    if(g_editor.screen.curline && g_editor.screen.curline->gb) {
      curpos = gap_buffer_getcursor(g_editor.screen.curline->gb);
    }

    g_editor.screen.curline = g_editor.screen.lastline;
    g_editor.screen.curline_number = g_editor.screen.lastline_number;

    if(g_editor.screen.curline && g_editor.screen.curline->gb) {
      int new_curpos = gap_buffer_getcursor(g_editor.screen.curline->gb);
      curpos = new_curpos < curpos ? new_curpos : curpos;
    } else {
      curpos = 0;
    }

    g_editor.screen.curline_cursor = curpos;

  }

  editor_redraw_main_window_full();

  return 0;
}

void editor_refresh_windows() {

  wnoutrefresh(stdscr);
  wnoutrefresh(g_windows.mainwnd);
  wnoutrefresh(g_windows.statuswnd);
  wnoutrefresh(g_windows.inputwnd);

  editor_place_cursor();

  doupdate();
}

void editor_redraw_main_window_full() {
  int mx, my, cx, cy, cur_line = 0;

  werase(g_windows.mainwnd);

  if(!g_editor.screen.linebuf) {
    editor_refresh_windows();
    return;
  }

  getmaxyx(g_windows.mainwnd, my, mx);
  wmove(g_windows.mainwnd, 0, 0);

  struct editor_line *el = g_editor.screen.firstline;
  char *linebuf = g_editor.screen.linebuf;
  size_t linebuf_len = g_editor.screen.linebuf_len;

  cx = 0;
  while(el && cur_line < my) {
    int n_copied = gap_buffer_copy(el->gb, linebuf, linebuf_len);
    for(int i = 0; i < n_copied; ++i) {
      if(isprint(linebuf[i])) {
        waddch(g_windows.mainwnd, linebuf[i]);
      } else {
        waddch(g_windows.mainwnd, ' ');
      }
    }

    if(wmove(g_windows.mainwnd, ++cx, 0) == ERR) {
      break;
    }
    el = el->next;
    ++cur_line;
  }

  int curs_pos = g_editor.screen.curline_cursor >= 0 ? g_editor.screen.curline_cursor : 0; 
  if(g_editor.screen.curline 
      && g_editor.screen.curline->gb) 
  {
    gap_buffer_setcursor(g_editor.screen.curline->gb, curs_pos);
    curs_pos = curs_pos < gap_buffer_getcursor(g_editor.screen.curline->gb) 
                ? curs_pos 
                : gap_buffer_getcursor(g_editor.screen.curline->gb);
  }

  if(wmove(g_windows.mainwnd, 
            g_editor.screen.curline_number - g_editor.screen.firstline_number, // should always be < my
            curs_pos < mx ? curs_pos : mx - 1) == ERR) 
  {
    LOG_MSG("Moving cursor to %d,%d failed, width: %d, height: %d", g_editor.screen.curline_number - g_editor.screen.firstline_number, curs_pos < mx ? curs_pos : mx, getmaxx(stdscr), getmaxy(stdscr));
  }

  editor_refresh_windows();
}

long editor_get_top_line() {
  return g_editor.screen.firstline_number;
}

long editor_goto_line_scan(long dest_line) {
  struct editor_line *el = g_editor.screen.firstline 
                            ? g_editor.screen.firstline 
                            : g_editor.data.firstline;
  if(dest_line < 0) {
    dest_line = 0;
  }

  long cur_line = 0;
  while(el && el->next && cur_line < dest_line) {
    el = el->next;
    ++cur_line;
  }
  g_editor.screen.firstline = el;
  g_editor.screen.firstline_number = cur_line;

  // lastline is inclusive, so we want to stop at height - 1 additonal lines
  dest_line += g_windows.mainwnd_geom.h ? g_windows.mainwnd_geom.h - 1 : 0; 

  long new_curline_number = g_editor.screen.curline_number;

  if(new_curline_number < g_editor.screen.firstline_number) {
    new_curline_number = g_editor.screen.firstline_number;
  } else if(g_editor.screen.curline_number > dest_line) {
    new_curline_number = dest_line;
  }

  while(el && el->next && cur_line < dest_line) {
    if(cur_line <= new_curline_number) {
      g_editor.screen.curline = el;
      g_editor.screen.curline_number = cur_line;
    }
    el = el->next;
    ++cur_line;
  }

  g_editor.screen.lastline = el;
  g_editor.screen.lastline_number = cur_line;

  return g_editor.screen.firstline_number;
}

void editor_char_left_main() {
  if(!g_editor.screen.curline || !g_editor.screen.curline->gb) {
    return;
  }
  gap_buffer *gb = g_editor.screen.curline->gb;

  int buf_len = gap_buffer_length(gb);
  if(g_editor.screen.curline_cursor > buf_len) {
    g_editor.screen.curline_cursor = buf_len;
  }

  if(g_editor.screen.curline_cursor > 0) {
    --g_editor.screen.curline_cursor;
    gap_buffer_setcursor(gb, g_editor.screen.curline_cursor);
    g_editor.screen.curline_cursor = gap_buffer_getcursor(gb);
  }
  editor_update_cursor_main();
}

void editor_char_right_main() {
  if(!g_editor.screen.curline || !g_editor.screen.curline->gb) {
    return;
  }

  gap_buffer *gb = g_editor.screen.curline->gb;

  int buf_len = gap_buffer_length(gb);
  if(g_editor.screen.curline_cursor > buf_len) {
    return;
  }
  
  int pos = g_editor.screen.curline_cursor;
  if(pos < g_windows.mainwnd_geom.w - 1 ? g_windows.mainwnd_geom.w - 2 : 0) {
    ++pos;
    gap_buffer_setcursor(gb, pos);
    g_editor.screen.curline_cursor = gap_buffer_getcursor(gb);
  }
  editor_update_cursor_main();
}

// moves the current line down one line
void editor_line_down_main() {
  struct editor_line *el = g_editor.screen.curline;
  size_t ln = g_editor.screen.curline_number;
  if(!el || !el->next) {
    return;
  }
  el = el->next;
  ++ln;
  
  // if we're on the last line, scroll the screen down (if possible)
  if(g_editor.screen.curline == g_editor.screen.lastline) {
    if(g_editor.screen.lastline->next) {
      g_editor.screen.firstline = g_editor.screen.firstline->next;
      ++g_editor.screen.firstline_number;
    }
    g_editor.screen.lastline = el;
    g_editor.screen.lastline_number = ln;
  }

  g_editor.screen.curline = el;
  g_editor.screen.curline_number = ln;
  gap_buffer_setcursor(g_editor.screen.curline->gb, g_editor.screen.curline_cursor);

  editor_update_cursor_main();
}

void editor_line_up_main() {
  struct editor_line *el = g_editor.screen.curline;
  size_t ln = g_editor.screen.curline_number;
  if(!el || !el->prev) {
    return;
  }
  el = el->prev;
  --ln;
  
  // if we're on the last line, scroll the screen down (if possible)
  if(g_editor.screen.curline == g_editor.screen.firstline) {
    if(g_editor.screen.firstline->prev) {
      g_editor.screen.lastline = g_editor.screen.lastline->prev;
      --g_editor.screen.lastline_number;
    }
    g_editor.screen.firstline = el;
    g_editor.screen.firstline_number = ln;
  }

  g_editor.screen.curline = el;
  g_editor.screen.curline_number = ln;
  gap_buffer_setcursor(g_editor.screen.curline->gb, g_editor.screen.curline_cursor);

  editor_update_cursor_main();
}

void editor_update_cursor_main() {
  editor_redraw_main_window_full();
}
