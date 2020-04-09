#include <errno.h>
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
  IpcClient *client = (IpcClient *)clientptr;
  uint16_t procs = get_procs(client->store);

  Chan *chans[procs];
  for (int i = 0; i < procs; i++) {
    if (i != client->id) {
      chans[i] = get_chan(client->store, i, client->id);
      if (set_nonblock_chan(chans[i]) == -1) {
        return FAILED;
      }
    }
  }

  for (int i = 0;; i = (i + 1) % procs) {
    if (i != client->id) {
      Chan *chan = chans[i];
      size_t size = sizeof(MessageHeader);

      size_t n = read_chan(chan, &msg->s_header, size);
      if (n != size) {
        if (errno != EAGAIN) {
          return FAILED;
        }
        usleep(1);
        continue;
      }

      size = msg->s_header.s_payload_len;
      if (read_chan(chan, msg->s_payload, size) != size) {
        return FAILED;
      }
      break;
    }
  }

  for (int i = 0; i < procs; i++) {
    if (i != client->id) {
      if (set_block_chan(chans[i]) == -1) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}
