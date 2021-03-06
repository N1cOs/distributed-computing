#ifndef __IFMO_DISTRIBUTED_CLASS_PROC__H
#define __IFMO_DISTRIBUTED_CLASS_PROC__H

#include "clock.h"
#include "ipc.h"
#include "pa2345.h"
#include "stdbool.h"
#include "store.h"

typedef struct {
  local_id id;
  Store *store;
} IpcClient;

typedef struct {
  timestamp_t time;
  local_id proc;
} Tuple;

typedef enum {
  RCV_ALL_OK = 0,
  RCV_ALL_TRANSPORT,
  RCV_ALL_BAD_RESPONSE
} ReceiveAllError;

void init_client(IpcClient *client);

uint16_t get_left_siblings(IpcClient *client);

ReceiveAllError receive_from_all(IpcClient *client, MessageType type);

const char *str_receive_error(ReceiveAllError err);

#endif  // __IFMO_DISTRIBUTED_CLASS_PROC__H
