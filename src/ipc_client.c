#include "ipc_client.h"

static const char *err_msgs[] = {
    "ok",
    "receive",
    "received bad message",
};

static bool _left_siblings[MAX_PROCESS_ID];

static uint16_t _get_count(bool arr[], uint16_t size) {
  uint16_t left = 0;
  for (uint16_t id = 0; id < size; id++) {
    if (arr[id]) {
      left++;
    }
  }
  return left;
}

void init_client(IpcClient *client) {
  for (local_id id = PARENT_ID + 1; id < get_procs(client->store); id++) {
    if (id != client->id) {
      _left_siblings[id] = true;
    }
  }
  close_unnes_chans(client->store, client->id);
}

uint16_t get_left_siblings(IpcClient *client) {
  return _get_count(_left_siblings, get_procs(client->store));
}

int request_cs(const void *clientptr) {
  IpcClient *client = (IpcClient *)clientptr;

  increment_lamprot_time();
  Tuple req = {get_lamport_time(), client->id};
  add(client->queue, req);

  MessageHeader header = {MESSAGE_MAGIC, 0, CS_REQUEST, req.time};
  Message msg = {header};

  for (local_id id = PARENT_ID + 1; id < get_procs(client->store); id++) {
    if (_left_siblings[id]) {
      if (send(client, id, &msg) != 0) {
        return -1;
      }
    }
  }

  uint16_t procs = get_procs(client->store);
  bool left_reply[procs];
  for (local_id id = PARENT_ID + 1; id < procs; id++) {
    left_reply[id] = _left_siblings[id];
  }

  while (_get_count(left_reply, procs) != 0 ||
         !equal_tuples(req, top(client->queue))) {
    local_id id;
    if ((id = receive_any(client, &msg)) == -1) {
      return -1;
    }
    align_lamport_time(msg.s_header.s_local_time);

    switch (msg.s_header.s_type) {
      case CS_REPLY:
        left_reply[id] = false;
        break;
      case CS_REQUEST: {
        Tuple rcvd = {msg.s_header.s_local_time, id};
        add(client->queue, rcvd);

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
        pop(client->queue);
        break;
      case DONE:
        if (left_reply[id]) {
          left_reply[id] = false;
        }
        _left_siblings[id] = false;
        break;
      default:
        return -1;
    }
  }
  return 0;
}

int release_cs(const void *clientptr) {
  IpcClient *client = (IpcClient *)clientptr;
  pop(client->queue);

  increment_lamprot_time();
  MessageHeader hdr = {MESSAGE_MAGIC, 0, CS_RELEASE, get_lamport_time()};
  Message msg = {hdr};

  for (local_id id = PARENT_ID + 1; id < get_procs(client->store); id++) {
    if (_left_siblings[id]) {
      if (send(client, id, &msg) != 0) {
        return -1;
      }
    }
  }
  return 0;
}

const char *str_receive_error(ReceiveAllError err) { return err_msgs[err]; }

ReceiveAllError receive_from_all(IpcClient *client, MessageType type) {
  Message rcv_msg;
  uint16_t procs = get_procs(client->store);
  for (local_id id = PARENT_ID + 1; id < procs; id++) {
    if (id != client->id) {
      if (receive(client, id, &rcv_msg) != 0) {
        return RCV_ALL_TRANSPORT;
      }

      MessageHeader hdr = rcv_msg.s_header;
      if (hdr.s_magic != MESSAGE_MAGIC && hdr.s_type != type) {
        return RCV_ALL_BAD_RESPONSE;
      }

      align_lamport_time(hdr.s_local_time);
    }
  }
  return RCV_ALL_OK;
}
