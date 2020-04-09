#include "bank_client.h"
#include <stdio.h>
#include <string.h>
#include "banking.h"

void transfer(void* bank_client_ptr, local_id src, local_id dst,
              balance_t amount) {
  size_t transfer_order_size = sizeof(TransferOrder);

  TransferOrder transfer_order = {src, dst, amount};

  MessageHeader header = {MESSAGE_MAGIC, transfer_order_size, TRANSFER,
                          get_physical_time()};
  Message msg = {header};
  memcpy(msg.s_payload, &transfer_order, transfer_order_size);

  BankClient* bank_client = (BankClient*)bank_client_ptr;
  IpcClient* client = (IpcClient*)bank_client->client;

  if (send(client, src, &msg) != 0) {
    fprintf(stderr, "error: sending to process %1d\n", src);
    return;
  }

  if (receive(client, dst, &msg) != 0) {
    fprintf(stderr, "error: receiving from process %1d\n", dst);
    return;
  }

  if (header.s_type != ACK) {
    fprintf(stderr, "error: bad type from process %1d: expected ACK type\n", dst);
    return;
  }
}