#ifndef __GAP_BUFFER_H__
#define __GAP_BUFFER_H__

#include <stdlib.h>
#include <stdbool.h>

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

gap_buffer *gap_buffer_new(size_t len);

void gap_buffer_delete(gap_buffer *gb);

bool gap_buffer_empty(gap_buffer *gb);

size_t gap_buffer_length(gap_buffer *gb);

// sync up the gap with the cursor
int _gap_buffer_sync(gap_buffer *gb);

// checks if the buffer should be expanded
int _gap_buffer_chkexpand(gap_buffer *gb);

// shrink the storage allocated for the gap buffer if it's below a low watermark
int _gap_buffer_chkshrink(gap_buffer *gb);

// move the cursor by a relative amount (will be >= 0 and <= len)
int gap_buffer_movepos(gap_buffer *gb, int count);

// set the cursor position within the buffer (checks to be sure final position >= 0 and <= len)
int gap_buffer_setpos(gap_buffer *gb, int pos);

// return the number of characters added to the buffer
int gap_buffer_addch(gap_buffer *gb, char c);

// delete a single character before the cursor position
int gap_buffer_delch(gap_buffer *gb);

int gap_buffer_copy(gap_buffer *gb, char *dst, size_t len);

char gap_buffer_getbyte(gap_buffer *gb, size_t pos);
char *gap_buffer_getpos(gap_buffer *gb, size_t pos);

#endif // __GAP_BUFFER_H__
