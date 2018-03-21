#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "gap_buffer.h"

struct _dbuffer { // data buffer
  gap_buffer *buffer;
};

typedef struct _dbuffer dbuffer;

#endif __BUFFER_H__
