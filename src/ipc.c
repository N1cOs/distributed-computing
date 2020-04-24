#include "ipc_client.h"

#define SUCCESS 0
#define FAILED -1

int send(void *clientptr, local_id dst, const Message *msg) {
  IpcClient *client = ((IpcClient *)clientptr);
  Chan *chan = get_chan(client->store, client->id, dst);
  if (chan == NULL) {
    return FAILED;
  }

  size_t n;
  size_t size = sizeof(MessageHeader) + msg->s_header.s_payload_len;
  if ((n = write_chan(chan, msg, size)) != size) {
    return FAILED;
  }
  return SUCCESS;
}

int send_multicast(void *clientptr, const Message *msg) {
  IpcClient *client = ((IpcClient *)clientptr);
  uint16_t procs = get_procs(client->store);
  for (uint16_t id = 0; id < procs; id++) {
    if (id != client->id) {
      if (send(client, id, msg) == FAILED) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

int receive(void *clientptr, local_id from, Message *msg) {
  IpcClient *client = ((IpcClient *)clientptr);
  Chan *chan = get_chan(client->store, from, client->id);
  if (chan == NULL) {
    return FAILED;
  }

  size_t n;
  size_t size = sizeof(MessageHeader);
  if ((n = read_chan(chan, &msg->s_header, size)) != size) {
    return FAILED;
  }

  size = msg->s_header.s_payload_len;
  if ((n = read_chan(chan, msg->s_payload, size)) != size) {
    return FAILED;
  }
  return SUCCESS;
}

int receive_any(void *clientptr, Message *msg) {
  // Not implemented because of blocking IO.
  return FAILED;
}
