#include "store.h"
#include <fcntl.h>

Store* new_store(uint16_t procs) {
  ChanTable* table = new_table(procs, procs);
  for (uint16_t i = 0; i < table->height; i++) {
    for (uint16_t j = 0; j < table->width; j++) {
      if (i != j) {
        int pipefd[2];
        if (pipe(pipefd) != 0) {
          return NULL;
        }

        int fd, flags;
        for (int i = 0; i < 2; i++) {
          fd = pipefd[i];
          flags = fcntl(fd, F_GETFL, 0);
          if (flags == -1) {
            return NULL;
          }
          if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            return NULL;
          }
        }

        Chan chan = {.rfd = pipefd[0], .wfd = pipefd[1]};
        table->data[i][j] = chan;
      }
    }
  }

  Store* store = malloc(sizeof(Store));
  store->table = table;
  return store;
}

Chan* get_chan(Store* store, local_id src, local_id dst) {
  ChanTable* table = store->table;
  if (src > table->height || dst > table->width || src == dst) {
    return NULL;
  }
  return &table->data[src][dst];
}

void close_unnes_chans(Store* store, local_id left) {
  ChanTable* table = store->table;
  for (uint16_t src = 0; src < table->height; src++) {
    for (uint16_t dst = 0; dst < table->width; dst++) {
      if (src != dst) {
        Chan* chan = &table->data[src][dst];
        if (left == src) {
          close(chan->rfd);
        } else if (left == dst) {
          close(chan->wfd);
        } else {
          close_chan(chan);
        }
      }
    }
  }
}

uint16_t get_procs(Store* store) { return store->table->width; }

void free_store(Store* store) {
  ChanTable* table = store->table;
  for (uint16_t i = 0; i < table->height; i++) {
    for (uint16_t j = 0; j < table->width; j++) {
      if (i != j) {
        close_chan(&table->data[i][j]);
      }
    }
  }
  free_table(table);
  free(store);
}
