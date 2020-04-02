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
    }
  }
  return RCV_ALL_OK;
}
