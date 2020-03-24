#include "proc.h"

#define SUCCESS 0
#define FAILED -1

int send(void *procptr, local_id dst, const Message *msg) {
  Proc *proc = ((Proc *)procptr);
  Chan *chan = get_chan(proc->store, proc->id, dst);
  if (chan == NULL) {
    return FAILED;
  }

  size_t n;
  size_t size = sizeof(MessageHeader);
  if ((n = write_chan(chan, &msg->s_header, size)) != size) {
    return FAILED;
  }

  size = msg->s_header.s_payload_len;
  if ((n = write_chan(chan, msg->s_payload, size)) != size) {
    return FAILED;
  }
  return SUCCESS;
}

int send_multicast(void *procptr, const Message *msg) {
  Proc *proc = ((Proc *)procptr);
  uint16_t procs = get_procs(proc->store);
  for (uint16_t id = 0; id < procs; id++) {
    if (id != proc->id) {
      if (send(proc, id, msg) == FAILED) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

int receive(void *procptr, local_id from, Message *msg) {
  Proc *proc = ((Proc *)procptr);
  Chan *chan = get_chan(proc->store, from, proc->id);
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

int receive_any(void *procptr, Message *msg) {
  // Not implemented because of blocking IO.
  return FAILED;
}
