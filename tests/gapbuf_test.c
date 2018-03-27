#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <malloc.h>
#include "../gap_buffer.h"

#define check_assert_with_fail(expr, msg, fail) \
 do { \
   if(!(expr)) { \
     fprintf(stderr, "FAIL %s line %d: %s\n", __FILE__, __LINE__, msg); \
     if(fail) { \
       exit(-1); \
     } \
   } else { \
     fprintf(stderr, "OK   %s line %d: %s\n", __FILE__, __LINE__, msg); \
   } \
 } while(false);

#define check_assert(expr, msg) check_assert_with_fail(expr, msg, false)
#define fail_assert(expr, msg) check_assert_with_fail(expr, msg, true)

void print_gap_buf(gap_buffer *p) {
  printf("buf: %p (allocated: %lu), len: %lu, maxlen: %lu, gap_begin: %d, gap_end: %d, cursor: %d\n",
      p->buf, malloc_usable_size(p->buf), p->len, p->maxlen, p->gap_begin, p->gap_end, p->cursor);
  for(int i = 0; i < p->maxlen; ++i) {
    if(isprint(p->buf[i]) && (i < p->gap_begin || i >= p->gap_end)) {
      putchar(p->buf[i]);
    } else {
      putchar('.');
    }
  }
  putchar('\n');

}

void test_addmove() {
  printf("\n\ntest_addmove\n");
  const int initlen = 16;
  gap_buffer *p = gap_buffer_new(initlen);

  check_assert(p->cursor == 0, "init cursor 0");
  check_assert(p->len == 0, "init len 0");
  check_assert(p->maxlen == initlen, "init maxlen 64");
  check_assert(p->gap_begin == 0, "init gap_begin 0");
  check_assert(p->gap_end - p->gap_begin == initlen, "gap is appropriate length");

  check_assert(gap_buffer_addch(p, 'a') == 1, "gap buffer adding character");

  //print_gap_buf(p);

  check_assert(p->cursor == 1, "after addch cursor == 1");
  check_assert(p->gap_begin == 1, "after addch gap_begin == 1");

  gap_buffer_movepos(p, -1);
  check_assert(p->cursor == 0, "after move cursor == 0");
  check_assert(p->gap_begin == 1, "after move gap_begin == 1");

  gap_buffer_addch(p, 'b');

  //check_assert(strncmp(p->buf, "ba", 2) == 0, "internal buffer matches expected values");
  check_assert(gap_buffer_getbyte(p, 0) == 'b', "buf[0] == b (expected value)");
  check_assert(gap_buffer_getbyte(p, 1) == 'a', "buf[1] == a (expected value)");

  gap_buffer_delete(p);
}

void test_delete() {
  printf("\n\ntest_delete\n");
  gap_buffer *p = gap_buffer_new(0);

  check_assert(gap_buffer_delch(p) == 0, "delch on empty buffer");
  //print_gap_buf(p);

  gap_buffer_addch(p, 'a');

  check_assert(p->maxlen == 1, "after add maxlen == 1");
  check_assert(gap_buffer_length(p) == 1, "after add len == 1");
  check_assert(p->gap_begin == 1, "after add len == 1");
  check_assert(p->cursor == 1, "after add cursor == 1");

  //print_gap_buf(p);
  gap_buffer_delch(p);
  //print_gap_buf(p);
  gap_buffer_addch(p, 'a');
  //print_gap_buf(p);

  gap_buffer_delch(p);
  print_gap_buf(p);
  gap_buffer_addch(p, 'c');
  gap_buffer_addch(p, 'c');
  print_gap_buf(p);
  gap_buffer_addch(p, 'c');
  print_gap_buf(p);

  gap_buffer_setpos(p, 1);
  _gap_buffer_sync(p);
  gap_buffer_delch(p);
  print_gap_buf(p);

  gap_buffer_delete(p);
}

void test_shrink() {
  printf("\n\ntest_shrink\n");
  gap_buffer *p = gap_buffer_new(8);
  //print_gap_buf(p);

  for(int i = 0; i < p->maxlen; ++i) {
    gap_buffer_addch(p, 'a');
  }
  //print_gap_buf(p);

  for(int i = 0; i < 6; ++i) {
    gap_buffer_delch(p);
  }
  check_assert(p->maxlen == 4, "shrinking works");

  for(int i = 2; i < 12; ++i) {
    gap_buffer_addch(p, 'a');
  }
  check_assert(p->len == 12, "adding lots of chars after shrinking");

  gap_buffer_delete(p);
}

void test_copy() {
  printf("\n\ntest_copy\n");

  char buf[64] = { 0 }, chkbuf[9] = { 0 };
  gap_buffer *p = gap_buffer_new(8);

  for(int i = 'a'; i < 'a' + 8; ++i) {
    gap_buffer_addch(p, i);
    chkbuf[i - 'a'] = i;
  }
  //print_gap_buf(p);
  gap_buffer_copy(p, buf, 64);

  check_assert(strcmp(buf, chkbuf) == 0, "copy");

  gap_buffer_setpos(p, 4);
  _gap_buffer_sync(p);
  memset(buf, 0, 64);
  gap_buffer_copy(p, buf, 64);

  check_assert(strcmp(buf, chkbuf) == 0, "copy after setpos");

  memset(buf, 0, 64);
  gap_buffer_copy(p, buf, 4);
  check_assert(strncmp(buf, chkbuf, 4) == 0, "copy to short buffer");

  gap_buffer_delete(p);
}

int main(int argc, char *argv[]) {

  test_addmove();
  test_delete();
  test_shrink();
  test_copy();

  return 0;
}
