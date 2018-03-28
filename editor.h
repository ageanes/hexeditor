#ifndef __EDITOR_H__
#define __EDITOR_H__

#include "gap_buffer.h"

#include <ncurses.h>

struct _editor_window_geom {
  int x, y; // absolute x and y coords of upper left corner
  int w, h; // width and height of window
};

struct _editor_window {
  WINDOW *mainwnd;
  struct _editor_window_geom mainwnd_geom;

  WINDOW *statuswnd;
  struct _editor_window_geom statuswnd_geom;

  WINDOW *inputwnd;
  struct _editor_window_geom inputwnd_geom;
};
typedef struct _editor_window editor_window;

extern editor_window g_windows;

// curses windows
struct _file_info {
  const char *filename;
  FILE *fp;       // file pointer
  int fd;         // file descriptor
  char *mm;       // mmapped area
  long mm_offset; // offset into file that is mmaped
  long mm_len;    // length of memory map
}; 
typedef struct _file_info file_info;

extern file_info g_curfile;

struct editor_line {
  gap_buffer *gb; // gap buffer for writing characters

  struct editor_line *next;
  struct editor_line *prev;
};

struct _editor_status {
  int mode;
  struct {
    struct editor_line *lines;
    struct editor_line *firstline; // should be same as lines
    struct editor_line *lastline;
    size_t n_lines;
  } data;

  struct {
    WINDOW *focus;
    struct editor_line *firstline; // first line displayed on the screen
    size_t firstline_number;
    struct editor_line *lastline; // last line displayed on the screen
    size_t lastline_number;
    struct editor_line *curline; // line holding the current cursor
    size_t curline_number;
    size_t curline_cursor; // where on the line the cursor should be placed

    // a buffer that is the width of the windows
    char *linebuf;
    size_t linebuf_len;
  } screen;

};
typedef struct _editor_status editor_status;

extern editor_status g_editor;

enum _command_modes {
  MODE_COMMAND = 0,
  MODE_INSERT,
  NUMBER_MODES
};

struct _mode_ops {
  const char *mode_name;
  int (*enter_mode)();
  int (*new_char)(int c);
  int (*exit_mode)();
};
typedef struct _mode_ops mode_ops;

extern mode_ops g_modes[];

// prototypes

// setup 
int setup_curses();
int setup_windows();
int open_file(const char *filename);
int setup_editor();

// cleanup
void cleanup_editor();
void cleanup_file();
void cleanup_windows();
void cleanup_curses();

void editor_set_focus(WINDOW *window);
void editor_place_cursor();

// functionality
void clear_status_window();
void set_status_window_text(const char *str);
void append_status_window_text(const char *str);

void clear_input_window();
void set_input_window_text(const char *str);
void append_input_window_text(const char *str);
void append_input_window_char(char c);

int editor_switch_mode(int to_mode);

void editor_update_window_sizes();
int editor_resize_windows();
void editor_refresh_windows();
void editor_redraw_main_window_full();

long editor_get_top_line(); // get the first line displayed on the editor window

// set 'line' as the first line on the screen.
// return the first line displayed on the screen (may be < line)
long editor_goto_line_scan(long line); 
void editor_char_left_main();
void editor_char_right_main();
void editor_line_down_main();
void editor_line_up_main();

void editor_update_line();
void editor_update_cursor_main();

#endif // __EDITOR_H__
