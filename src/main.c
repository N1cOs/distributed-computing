#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"
#include "log.h"
#include "pa1.h"
#include "proc.h"

#define MAX_PROCS 10

char* build_msg(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t size = vsnprintf(NULL, 0, fmt, args);
  if (size < 0) {
    return NULL;
  }
  va_end(args);

  char* str = malloc(size + 1);
  va_start(args, fmt);

  vsnprintf(str, size + 1, fmt, args);
  va_end(args);

  return str;
}

int execute_child(Proc* proc, Log* log) {
  init_proc(proc);

  char* start_msg = build_msg(log_started_fmt, proc->id, getpid(), getppid());
  flog(log, start_msg);

  MessageHeader header = {MESSAGE_MAGIC, strlen(start_msg), STARTED, 0};
  Message msg = {header};
  strcpy(msg.s_payload, start_msg);

  if (send_multicast(proc, &msg) != 0) {
    fprintf(stderr, "error: process %1d: %s: %s\n", proc->id, "send_multicast",
            "start");
    free(start_msg);
    return EXIT_FAILURE;
  }
  free(start_msg);

  ReceiveAllError err;
  if ((err = receive_from_all(proc, STARTED)) != RCV_ALL_OK) {
    fprintf(stderr, "error: process %1d: %s\n", proc->id,
            str_receive_error(err));
    return EXIT_FAILURE;
  }
  flog(log, log_received_all_started_fmt, proc->id);

  char* done_msg = build_msg(log_done_fmt, proc->id);
  flog(log, done_msg);

  header.s_payload_len = strlen(done_msg);
  header.s_type = DONE;
  msg.s_header = header;
  strcpy(msg.s_payload, done_msg);

  if (send_multicast(proc, &msg) != 0) {
    fprintf(stderr, "error: process %1d: %s: %s\n", proc->id, "send_multicast",
            "done");
    free(done_msg);
    return EXIT_FAILURE;
  }
  free(done_msg);

  if ((err = receive_from_all(proc, DONE)) != RCV_ALL_OK) {
    fprintf(stderr, "error: process %1d: %s\n", proc->id,
            str_receive_error(err));
    return EXIT_FAILURE;
  }
  flog(log, log_received_all_done_fmt, proc->id);

  return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
  int opt;
  uint16_t procs = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1) {
    switch (opt) {
      case 'p':
        procs = strtoul(optarg, NULL, 10);
        if (procs > MAX_PROCS) {
          fprintf(stderr, "%s: %s: %d\n", argv[0],
                  "number of processes can't be more than", MAX_PROCS);
          return EXIT_FAILURE;
        }
        break;
      default:
        fprintf(stderr, "usage: main -p PROCS_NUM\n");
        return EXIT_SUCCESS;
    }
  }

  if (procs == 0) {
    fprintf(stderr, "%s: %s\n", argv[0],
            "number of processes have to be greater than 0");
    return EXIT_FAILURE;
  }
  procs++;

  FILE* eventsf = fopen(events_log, "w");
  if (eventsf == NULL) {
    fprintf(stderr, "%s: %s: %s\n", argv[0], "fopen", strerror(errno));
    return EXIT_FAILURE;
  }
  Log* log = new_log(2, stdout, eventsf);

  Store* store = new_store(procs);
  if (store == NULL) {
    fprintf(stderr, "%s: %s\n", argv[0], "new_store");
    return EXIT_FAILURE;
  }

  for (local_id id = PARENT_ID + 1; id < procs; id++) {
    pid_t pid = fork();
    if (pid == 0) {
      Proc proc = {id, store};
      int status = execute_child(&proc, log);

      free_store(store);
      free_log(log);
      fclose(eventsf);

      return status;
    } else if (pid < 0) {
      fprintf(stderr, "%s: %s: %s\n", argv[0], "fork", strerror(errno));
      return EXIT_FAILURE;
    }
  }

  Proc proc = {PARENT_ID, store};
  init_proc(&proc);

  ReceiveAllError err;
  if ((err = receive_from_all(&proc, STARTED)) != RCV_ALL_OK) {
    fprintf(stderr, "error: process %1d: %s\n", proc.id,
            str_receive_error(err));
    return EXIT_FAILURE;
  }
  flog(log, log_received_all_started_fmt, proc.id);

  if ((err = receive_from_all(&proc, DONE)) != RCV_ALL_OK) {
    fprintf(stderr, "error: process %1d: %s\n", proc.id,
            str_receive_error(err));
    return EXIT_FAILURE;
  }
  flog(log, log_received_all_done_fmt, proc.id);

  free_store(store);
  free_log(log);
  fclose(eventsf);

  int status = 0;
  for (local_id i = 1; i < procs; i++) {
    int RCV_ALL_OK;
    wait(&RCV_ALL_OK);
    status |= RCV_ALL_OK;
  }
  return status;
}
