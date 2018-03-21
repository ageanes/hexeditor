#include "gap_buffer.h"

#include <string.h>

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

bool gap_buffer_empty(gap_buffer *gb) {
  return gb->len == 0;
}

size_t gap_buffer_length(gap_buffer *gb) {
  return gb->len;
}

// makes sure the gap beginning is at the cursor position
int _gap_buffer_sync(gap_buffer *gb) {
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

int _gap_buffer_chkexpand(gap_buffer *gb) {
  const int multiplier = 2;
  if(gb->gap_begin == gb->gap_end) {
    size_t newsize = gb->len ? gb->len * multiplier : 1;
    if(newsize < gb->len) { // check for overflow
      return -1;
    }

    void *newptr = realloc(gb->buf, newsize);
    if(!newptr) {
      return -1;
    }

    size_t move_ct = gb->len - gb->gap_end;
    memmove(newptr + newsize - move_ct, newptr + gb->gap_end, move_ct);
    gb->buf = newptr;
    gb->gap_end = newsize - move_ct;
    gb->maxlen = newsize;
  }
  return 0;
}

// shrink the storage allocated for the gap buffer if it's below a low watermark
int _gap_buffer_chkshrink(gap_buffer *gb) {
  const int divisor = 2;
  size_t low_watermark = gb->maxlen / (2 * divisor);
  size_t newlen = gb->maxlen / divisor;
  
  // avoid shrinking to zero to avoid realloc() acting as a free() call
  if(newlen && gb->len <= low_watermark) {
    int old_cursor = gb->cursor;

    // move the buffer gap to the end of the contents
    if(gap_buffer_setpos(gb, gb->len) != gb->len) {
      return -1;
    }
    _gap_buffer_sync(gb);

    char *newptr = realloc(gb->buf, newlen);
    if(!newptr) {
      gap_buffer_setpos(gb, old_cursor);
      return -1;
    }

    gb->buf = newptr;
    gb->maxlen = newlen;
    gb->gap_end = newlen;
    gap_buffer_setpos(gb, old_cursor);
  }
  return 0;
}

int gap_buffer_movepos(gap_buffer *gb, int count) {
  gb->cursor += count;
  if(gb->cursor < 0) {
    gb->cursor = 0;
  } else if(gb->cursor > gb->len) {
    gb->cursor = gb->len; }
  return gb->cursor;
}

int gap_buffer_setpos(gap_buffer *gb, int pos) {
  gb->cursor = pos;
  if(gb->cursor < 0) {
    gb->cursor = 0;
  } else if(gb->cursor > gb->len) {
    gb->cursor = gb->len;
  }
  return gb->cursor;
}

// return the number of characters added to the buffer
int gap_buffer_addch(gap_buffer *gb, char c) {
  _gap_buffer_sync(gb); // sync gap buffer with cursor
  if(_gap_buffer_chkexpand(gb) < 0) {
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

  _gap_buffer_sync(gb); // sync gap buffer with cursor
  if(gb->len == 0 || gb->cursor == 0) {
    return 0;
  }

  --gb->cursor;
  --gb->gap_begin;
  --gb->len;

  _gap_buffer_chkshrink(gb);

  return 1;
}

int gap_buffer_copy(gap_buffer *gb, char *dst, size_t len) {
  size_t n_copied = 0;
  size_t n_to_copy = gb->gap_begin;
  n_to_copy = n_to_copy < len ? n_to_copy : len;

  memcpy(dst, gb->buf, n_to_copy);
  dst += n_to_copy;

  len -= n_to_copy;
  n_copied += n_to_copy;
  n_to_copy = gb->len - n_to_copy;
  n_to_copy = n_to_copy < len ? n_to_copy : len;
  memcpy(dst, gb->buf + gb->gap_end, n_to_copy);
  n_copied += n_to_copy;

  return n_copied;
}

char gap_buffer_getbyte(gap_buffer *gb, size_t pos) {
  if(pos >= gb->gap_begin) {
    pos = (pos - gb->gap_begin) + gb->gap_end;
  }
  return gb->buf[pos];
}

char *gap_buffer_getpos(gap_buffer *gb, size_t pos) {
  if(pos >= gb->gap_begin) {
    pos = (pos - gb->gap_begin) + gb->gap_end;
  }
  return &gb->buf[pos];
}
