#ifndef __IFMO_DISTRIBUTED_CLASS_BANK_BRANCH__H
#define __IFMO_DISTRIBUTED_CLASS_BANK_BRANCH__H

#include "banking.h"
#include "ipc_client.h"
#include "log.h"
#include "pa2345.h"
#include "stdbool.h"

typedef struct {
  local_id id;
  IpcClient *client;
  BalanceHistory *history;
} BankBranch;

BankBranch *new_bank_branch(local_id id, Store *store);

int start_bank_branch(BankBranch *branch, balance_t balance, Log *log);

void free_bank_branch(BankBranch *branch);

#endif  // __IFMO_DISTRIBUTED_CLASS_BANK_BRANCH__H
