#ifndef __IFMO_DISTRIBUTED_CLASS_PROC__H
#define __IFMO_DISTRIBUTED_CLASS_PROC__H

#include "clock.h"
#include "ipc.h"
#include "pa2345.h"
#include "priority_queue.h"
#include "store.h"

typedef struct {
  local_id id;
  Store *store;
  PriorityQueue *queue;
} IpcClient;

typedef enum {
  RCV_ALL_OK = 0,
  RCV_ALL_TRANSPORT,
  RCV_ALL_BAD_RESPONSE
} ReceiveAllError;

void init_client(IpcClient *client);

ReceiveAllError receive_from_all(IpcClient *client, MessageType type);

const char *str_receive_error(ReceiveAllError err);

#endif  // __IFMO_DISTRIBUTED_CLASS_PROC__H
