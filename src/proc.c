#include "proc.h"

static const char *err_msgs[] = {
    "ok",
    "receive",
    "received bad message",
};

void init_proc(Proc *proc) { close_unnes_chans(proc->store, proc->id); }

const char *str_receive_error(ReceiveAllError err) { return err_msgs[err]; }

ReceiveAllError receive_from_all(Proc *proc, MessageType type) {
  Message rcv_msg;
  uint16_t procs = get_procs(proc->store);
  for (local_id id = PARENT_ID + 1; id < procs; id++) {
    if (id != proc->id) {
      if (receive(proc, id, &rcv_msg) != 0) {
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
