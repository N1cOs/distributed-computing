#ifndef __IFMO_DISTRIBUTED_CLASS_STORE__H
#define __IFMO_DISTRIBUTED_CLASS_STORE__H

#include <stdint.h>
#include <stdlib.h>
#include "chan.h"
#include "ipc.h"

typedef struct {
  ChanTable* table;
} Store;

Store* new_store(uint16_t procs);

Chan* get_chan(Store* store, local_id src, local_id dst);

void close_unnes_chans(Store* store, local_id left);

uint16_t get_procs(Store* store);

void free_store(Store* store);

#endif  // __IFMO_DISTRIBUTED_CLASS_STORE__H
