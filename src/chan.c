#include "chan.h"

ssize_t read_chan(Chan *chan, void *buf, size_t count) {
  size_t left = count;
  while (left != 0) {
    ssize_t n = read(chan->rfd, ((char *)buf) + (count - left), left);
    if (n < 0) {
      return count - left;
    }
    left -= n;
  }
  return count;
}

ssize_t write_chan(Chan *chan, const void *buf, size_t count) {
  size_t left = count;
  while (left != 0) {
    ssize_t n = write(chan->wfd, ((char *)buf) + (count - left), left);
    if (n < 0) {
      return count - left;
    }
    left -= n;
  }
  return count;
}

int close_chan(Chan *chan) { return close(chan->rfd) || close(chan->wfd); }

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
