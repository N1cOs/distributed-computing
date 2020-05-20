#include "ipc_client.h"

static const char *err_msgs[] = {
    "ok",
    "receive",
    "received bad message",
};

void init_client(IpcClient *client) {
  close_unnes_chans(client->store, client->id);
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

int request_cs(const void *clientptr) {
  IpcClient *client = (IpcClient *)clientptr;

  increment_lamprot_time();
  Tuple req = {get_lamport_time(), client->id};
  add(client->queue, req);

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
  while (left_reply != 0 && !equal_tuples(req, top(client->queue))) {
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
