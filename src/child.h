#ifndef __IFMO_DISTRIBUTED_CLASS_CHILD__H
#define __IFMO_DISTRIBUTED_CLASS_CHILD__H

#include "ipc_client.h"
#include "log.h"
#include "stdbool.h"
#include "store.h"

typedef struct {
  local_id id;
  IpcClient *client;
  // Mutex *mutex;
} Child;

Child *new_child(local_id id, Store *store);

int exec_child(Child *child, Log *log, bool mutexl);

int free_child(Child *child);

#endif  // __IFMO_DISTRIBUTED_CLASS_CHILD__H
