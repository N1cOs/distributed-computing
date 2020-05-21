#include "chan.h"

ssize_t read_chan(Chan *chan, void *buf, size_t count) {
  return read(chan->rfd, ((char *)buf), count);
}

ssize_t write_chan(Chan *chan, const void *buf, size_t count) {
  return write(chan->wfd, ((char *)buf), count);
}

int close_chan(Chan *chan) { return close(chan->rfd) || close(chan->wfd); }

int set_block_chan(Chan *chan) {
  int flags = fcntl(chan->rfd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  return fcntl(chan->rfd, F_SETFL, flags & (~O_NONBLOCK));
}

int set_nonblock_chan(Chan *chan) {
  int flags = fcntl(chan->rfd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  return fcntl(chan->rfd, F_SETFL, flags | O_NONBLOCK);
}

ChanTable *new_table(uint16_t width, uint16_t height) {
  ChanTable *table = malloc(sizeof(ChanTable));
  table->width = width;
  table->height = height;

  Chan **data = malloc(sizeof(Chan *) * height);
  for (uint16_t i = 0; i < height; i++) {
    data[i] = malloc(sizeof(Chan) * width);
  }
  table->data = data;

  return table;
}

void free_table(ChanTable *table) {
  for (uint16_t i = 0; i < table->height; i++) {
    free(table->data[i]);
  }
  free(table->data);
  free(table);
}
