#ifndef __EDITOR_H__
#define __EDITOR_H__

#include "gap_buffer.h"

#include <ncurses.h>

struct _editor_window {
  WINDOW *mainwnd;
  WINDOW *statuswnd;
  WINDOW *inputwnd;
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

    char *linebuf;
    size_t linebuf_len;
  } buf;

  struct editor_line *firstline; // first line on the screen
  size_t firstline_number;

  struct editor_line *curline; // current line
  size_t screen_curline;
};
typedef struct _editor_status editor_status;

extern editor_status g_editor_status;

enum _command_modes {
  MODE_COMMAND = 0,
  MODE_INSERT,
  NUMBER_MODES
};

struct _mode_ops {
  const char *mode_name;
  int (*enter_mode)();
  int (*new_char)(int c);
  int (*place_cursor)();
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

// functionality
void clear_status_window();
void set_status_window_text(const char *str);
void append_status_window_text(const char *str);

void clear_input_window();
void set_input_window_text(const char *str);
void append_input_window_text(const char *str);
void append_input_window_char(char c);

int editor_switch_mode(int to_mode);

int editor_resize_windows();
void editor_update_windows();
void editor_refresh_main_window();

void editor_update_line();

#endif // __EDITOR_H__
