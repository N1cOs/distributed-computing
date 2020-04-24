#ifndef __IFMO_DISTRIBUTED_CLASS_BANK_CLIENT__H
#define __IFMO_DISTRIBUTED_CLASS_BANK_CLIENT__H

#include "ipc_client.h"

typedef struct {
  IpcClient* client;
  local_id id;
} BankClient;

#endif  // __IFMO_DISTRIBUTED_CLASS_BANK_CLIENT__H
