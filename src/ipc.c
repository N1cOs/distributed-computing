#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <time.h>
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

  if (set_block_chan(chan) != 0) {
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

  if (set_nonblock_chan(chan) != 0) {
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
    }
  }

  struct timespec ts = {.tv_nsec = 10000};
  for (int i = 0;; i = (i + 1) % procs) {
    if (i != client->id) {
      Chan *chan = chans[i];
      size_t size = sizeof(MessageHeader);

      ssize_t n = read_chan(chan, &msg->s_header, size);
      if (n != size) {
        if (errno != EAGAIN) {
          return FAILED;
        }
        nanosleep(&ts, NULL);
        continue;
      }

      size = msg->s_header.s_payload_len;
      n = read_chan(chan, msg->s_payload, size);
      if (n != size) {
        return FAILED;
      }
      break;
    }
  }
  return SUCCESS;
}
