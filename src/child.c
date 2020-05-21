#include "child.h"

static char *_build_msg(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  int size = vsnprintf(NULL, 0, fmt, args);
  if (size < 0) {
    return NULL;
  }
  va_end(args);

  char *str = malloc(size + 1);
  va_start(args, fmt);

  vsnprintf(str, size + 1, fmt, args);
  va_end(args);

  return str;
}

static int _sync(Child *child, Message *msg) {
  if (send_multicast(child->client, msg) != 0) {
    fprintf(stderr, "error: child %1d: %s: %s\n", child->id, "send_multicast",
            "sync");
    return -1;
  }

  uint16_t left_reply = get_left_siblings(child->client);
  while (left_reply != 0) {
    Message rcvd_msg;
    if (receive_any(child->client, &rcvd_msg) == -1) {
      fprintf(stderr, "error: child %1d: %s: %s\n", child->id, "sync",
              "receive_any");
      return -1;
    }
    if (rcvd_msg.s_header.s_type == msg->s_header.s_type) {
      left_reply--;
    }
  }
  return 0;
}

Child *new_child(local_id id, Store *store) {
  IpcClient *client = malloc(sizeof(IpcClient));
  client->id = id;
  client->store = store;
  client->queue = new_prioriy_queue(16);

  Child *child = malloc(sizeof(Child));
  child->id = id;
  child->client = client;

  return child;
}

int exec_child(Child *child, bool mutexl) {
  init_client(child->client);

  increment_lamprot_time();

  timestamp_t start_time = get_lamport_time();
  MessageHeader start_header = {MESSAGE_MAGIC, 0, STARTED, start_time};
  Message start_msg = {start_header};

  if (_sync(child, &start_msg) != 0) {
    return EXIT_FAILURE;
  }

  for (int i = 0; i < child->id * 5; i++) {
    char *loop_msg =
        _build_msg(log_loop_operation_fmt, child->id, i + 1, child->id * 5);

    if (mutexl) {
      if (request_cs(child->client) != 0) {
        fprintf(stderr, "error: request_cs in child: %d\n", child->id);
        return EXIT_FAILURE;
      }
      print(loop_msg);
      if (release_cs(child->client) != 0) {
        fprintf(stderr, "error: release_cs in child: %d\n", child->id);
        return EXIT_FAILURE;
      }
    } else {
      print(loop_msg);
    }

    free(loop_msg);
  }

  increment_lamprot_time();

  timestamp_t done_time = get_lamport_time();
  MessageHeader done_header = {MESSAGE_MAGIC, 0, DONE, done_time};
  Message done_msg = {done_header};

  if (_sync(child, &done_msg) != 0) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void free_child(Child *child) {
  free_priority_queue(child->client->queue);
  free(child->client);
  free(child);
}
