#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <malloc.h>
#include "gap_buffer.h"

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

void test1() {
  printf("\n\ntest1\n");
  gap_buffer *p = gap_buffer_new(64);
  check_assert(p->cursor == 0, "init cursor 0");
  check_assert(p->len == 0, "init len 0");
  check_assert(p->maxlen == 64, "init maxlen 64");
  check_assert(p->gap_begin == 0, "init gap_begin 0");

  check_assert(gap_buffer_addch(p, 'a') == 1, "gap buffer adding character");

  print_gap_buf(p);

  check_assert(p->cursor == 1, "after addch cursor 1");
  check_assert(p->gap_begin == 1, "after addch gap_begin 1");

  gap_buffer_movepos(p, -1);
  check_assert(p->cursor == 0, "after move cursor 0");
  check_assert(p->gap_begin == 1, "after move gap_begin 1");

  gap_buffer_addch(p, 'b');
  print_gap_buf(p);

  gap_buffer_delete(p);
}

void test2() {
  printf("\n\ntest2\n");
  gap_buffer *p = gap_buffer_new(0);

  check_assert(gap_buffer_delch(p) == 0, "delch on empty buffer");
  print_gap_buf(p);

  gap_buffer_addch(p, 'a');

  check_assert(p->maxlen == 1, "after add maxlen 1");
  check_assert(p->len == 1, "after add len 1");
  check_assert(p->gap_begin == 1, "after add len 1");
  check_assert(p->cursor == 1, "after add cursor 1");
  print_gap_buf(p);
  gap_buffer_delch(p);
  print_gap_buf(p);
  gap_buffer_addch(p, 'a');
  print_gap_buf(p);

  gap_buffer_delch(p);
  print_gap_buf(p);
  gap_buffer_addch(p, 'c');
  gap_buffer_addch(p, 'c');
  print_gap_buf(p);
  gap_buffer_addch(p, 'c');
  print_gap_buf(p);

  gap_buffer_setpos(p, 1);
  gap_buffer_sync(p);
  gap_buffer_delch(p);
  print_gap_buf(p);

  gap_buffer_delete(p);
}

int main(int argc, char *argv[]) {

  test1();
  test2();

  return 0;
}
