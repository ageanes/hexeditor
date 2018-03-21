#ifndef __GAP_BUFFER_H__
#define __GAP_BUFFER_H__

// gap buffer:
// 
// contains data with a gap in the middle
//
//                                   cursor(2)
//                                   |
// aaaaaaaaaaaaaaaaaaa--------------aaaaa------*
// |             |    |            |
// buf           |    gap_begin     gap_end
//               |        
//               cursor (1)
//
// buf: holds data
// len: size of text in buffer
// maxlen: size of text + size of gap
// gap_begin: offset from buf where gap begins
// gap_end: offset from buf one past where the gap ends
// cursor: where the cursor is.
//          - on moving, the cursor acts as a placeholder for where the gap will end up
//          - on insertion/deletion, gap begin is moved to cursor before inserting/deleting
// 
struct _gap_buffer {
  char *buf;
  size_t len;
  size_t maxlen;
  int gap_begin;
  int gap_end;
  int cursor;

  struct _gap_buffer *next;
};

typedef struct _gap_buffer gap_buffer;

gap_buffer *gap_buffer_new(size_t len) {
  gap_buffer *new_gb = calloc(1, sizeof(gap_buffer));

  if(new_gb && len > 0) {
    new_gb->buf = malloc(len);
    if(new_gb->buf) {
      new_gb->maxlen = len;
      new_gb->len = 0;
      new_gb->gap_end = len;
    } else {
      free(new_gb);
      new_gb = NULL;
    }
  }
  return new_gb;
}

void gap_buffer_delete(gap_buffer *gb) {
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

// makes sure the gap beginning is at the cursor position
int gap_buffer_sync(gap_buffer *gb) {
  if(gb->gap_begin == gb->cursor) {
    return gb->gap_begin;
  }

  if(gb->cursor < gb->gap_begin) { // cursor is in front of the gap, need to move data back
    int count = gb->gap_begin - gb->cursor;
    memmove(gb->buf + gb->gap_end - count, gb->buf + gb->cursor, count);
    gb->gap_begin -= count;
    gb->gap_end -= count;
  } else { // cursor is in/past the gap, need to move data to front of buffer
    int count = gb->cursor - gb->gap_end;
    memmove(gb->buf + gb->gap_begin, gb->buf + gb->gap_end, count);
    gb->gap_begin += count;
    gb->gap_end += count;
    //gb->cursor = gb->gap_begin;
  }
  return gb->cursor;
}

int gap_buffer_chkexpand(gap_buffer *gb) {
  if(gb->gap_begin == gb->gap_end) {
    size_t newsize = gb->len ? gb->len * 2 : 1;
    if(newsize < gb->len) { // check for overflow
      return false;
    }

    void *newptr = realloc(gb->buf, newsize);
    if(!newptr) {
      return false;
    }

    size_t move_ct = gb->len - gb->gap_end;
    memmove(newptr + newsize - move_ct, newptr + gb->gap_end, move_ct);
    gb->buf = newptr;
    gb->gap_end = newsize - move_ct;
    gb->maxlen = newsize;
  }
  return true;
}

int gap_buffer_movepos(gap_buffer *gb, int count) {
  gb->cursor += count;
  if(gb->cursor < 0) {
    gb->cursor = 0;
  } else if(gb->cursor > gb->len) {
    gb->cursor = gb->len; }
  return gb->cursor;
}

int gap_buffer_setpos(gap_buffer *gb, int count) {
  gb->cursor = count;
  if(gb->cursor < 0) {
    gb->cursor = 0;
  } else if(gb->cursor > gb->len) {
    gb->cursor = gb->len;
  }
  return gb->cursor;
}

// return the number of characters added
int gap_buffer_addch(gap_buffer *gb, char c) {
  gap_buffer_sync(gb); // sync gap buffer with cursor
  if(!gap_buffer_chkexpand(gb)) {
    return 0;
  }
  if(gb->len >= gb->maxlen) {
    return 0;
  }

  gb->buf[gb->cursor] = c;
  ++gb->cursor;
  ++gb->gap_begin;
  ++gb->len;
  return 1;
}

int gap_buffer_delch(gap_buffer *gb) {
  gap_buffer_sync(gb); // sync gap buffer with cursor
  if(gb->len == 0 || gb->cursor == 0) {
    return 0;
  }

  --gb->cursor;
  --gb->gap_begin;
  --gb->len;
  //gb->buf[gb->cursor] = 0;

  return 1;
}


#endif // __GAP_BUFFER_H__
