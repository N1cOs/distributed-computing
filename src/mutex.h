#ifndef __IFMO_DISTRIBUTED_CLASS_MUTEX__H
#define __IFMO_DISTRIBUTED_CLASS_MUTEX__H

#include "clock.h"
#include "ipc_client.h"
#include "priority_queue.h"
#include "stdio.h"

typedef struct {
  PriorityQueue *queue;
} Mutex;

Mutex *new_mutex();

void free_mutex(Mutex *mutex);

int request_cs(Mutex *mutex, IpcClient *client);

int release_cs(Mutex *mutex, IpcClient *client);

#endif  // __IFMO_DISTRIBUTED_CLASS_MUTEX__H
