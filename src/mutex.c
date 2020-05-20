#include "mutex.h"

Mutex *new_mutex() {
  PriorityQueue *queue = new_prioriy_queue(16);
  if (queue == NULL) {
    return NULL;
  }

  Mutex *mu = malloc(sizeof(Mutex));
  if (mu == NULL) {
    return NULL;
  }

  mu->queue = queue;
  return mu;
}

void free_mutex(Mutex *mutex) {
  free_priority_queue(mutex->queue);
  free(mutex);
}

int request_cs(Mutex *mutex, IpcClient *client) {
  increment_lamprot_time();
  Tuple req = {get_lamport_time(), client->id};
  add(mutex->queue, req);

  MessageHeader header = {MESSAGE_MAGIC, 0, CS_REQUEST, req.time};
  Message msg = {header};

  uint16_t children = get_procs(client->store) - 1;
  for (local_id id = PARENT_ID + 1; id <= children; id++) {
    if (id != client->id) {
      if (send(client, id, &msg) != 0) {
        return -1;
      }
    }
  }

  uint16_t left_reply = children;
  while (left_reply != 0 && !equal_tuples(req, top(mutex->queue))) {
    local_id id;
    if ((id = receive_any(client, &msg)) == -1) {
      return -1;
    }
    align_lamport_time(msg.s_header.s_local_time);

    switch (msg.s_header.s_type) {
      case CS_REPLY:
        left_reply--;
        break;
      case CS_REQUEST: {
        Tuple rcvd = {msg.s_header.s_local_time, id};
        add(mutex->queue, rcvd);

        increment_lamprot_time();
        MessageHeader reply_hdr = {MESSAGE_MAGIC, 0, CS_REPLY,
                                   get_lamport_time()};
        Message reply_msg = {reply_hdr};

        if (send(client, id, &reply_msg) != 0) {
          return -1;
        }
        break;
      }
      case CS_RELEASE:
        pop(mutex->queue);
        break;
      default:
        return -1;
    }
  }
  return 0;
}

int release_cs(Mutex *mutex, IpcClient *client) {
  pop(mutex->queue);

  increment_lamprot_time();
  MessageHeader hdr = {MESSAGE_MAGIC, 0, CS_RELEASE, get_lamport_time()};
  Message msg = {hdr};

  uint16_t children = get_procs(client->store) - 1;
  for (local_id id = PARENT_ID + 1; id <= children; id++) {
    if (id != client->id) {
      if (send(client, id, &msg) != 0) {
        return -1;
      }
    }
  }
  return 0;
}
