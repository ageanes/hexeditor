#ifndef __GAP_BUFFER_H__
#define __GAP_BUFFER_H__

// gap buffer:
// 
// contains data with a gap in the middle
//
// aaaaaaaaaaaaaaaaaaa----_---------aaaaa------*
// |                  |   |         |
// buf                gap_begin     gap_end
//                        |
//                        cursor
//
// buf: holds data
// len: total size of buf
// gap_begin: offset from buf where gap begins
// gap_end: offset from buf one past where the gap ends
// cursor: where the cursor is.
//          - on moving, the cursor acts as a placeholder for where the gap will end up
//          - on insertion/deletion, gap begin is moved to cursor before inserting/deleting
// 
struct _gap_buffer {
  char *buf;
  size_t len;
  int gap_begin;
  int gap_end;
  int cursor;
};

typedef struct _gap_buffer gap_buffer;

gap_buffer *new_gap_buffer(size_t len) {
  gap_buffer *new_gb = calloc(1, sizeof(gap_buffer));

  if(new_gb && len > 0) {
    new_bg->buf = malloc(len);
    if(new_bg->buf) {
      new_bg->len = len;
      new_bg->gap_end = len;
      //new_bg->gap_begin = 0; // set by calloc
      //new_bg->cursor = 0;
    } else {
      free(new_gb);
      new_gb = NULL;
    }
  }
  return new_gb;
}

void del_gap_buffer(gap_buffer *gb) {
  if(gb) {
    if(gb->buf) {
      free(gb->buf);
    }
    free(gb);
  }
}

inline bool gap_buffer_empty(gap_buffer *gb) {
  return gb->len == 0;
}

int gap_buffer_advance(gap_buffer *gb, int count) {
}

#endif // __GAP_BUFFER_H__
