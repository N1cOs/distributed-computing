#ifndef __IFMO_DISTRIBUTED_CLASS_CHAN__H
#define __IFMO_DISTRIBUTED_CLASS_CHAN__H

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
  int rfd;
  int wfd;
} Chan;

typedef struct {
  uint16_t width;
  uint16_t height;
  Chan **data;
} ChanTable;

ssize_t read_chan(Chan *chan, void *buf, size_t count);

ssize_t write_chan(Chan *chan, const void *buf, size_t count);

int set_block_chan(Chan *chan);

int set_nonblock_chan(Chan *chan);

int close_chan(Chan *chan);

ChanTable *new_table(uint16_t width, uint16_t height);

void free_table(ChanTable *table);

#endif  // __IFMO_DISTRIBUTED_CLASS_CHAN__H
