#ifndef __IFMO_DISTRIBUTED_CLASS_CHILD__H
#define __IFMO_DISTRIBUTED_CLASS_CHILD__H

#include <stdio.h>
#include "ipc_client.h"
#include "stdbool.h"

typedef struct {
  local_id id;
  IpcClient *client;
} Child;

Child *new_child(local_id id, Store *store);

int exec_child(Child *child, bool mutexl);

void free_child(Child *child);

#endif  // __IFMO_DISTRIBUTED_CLASS_CHILD__H
