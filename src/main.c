#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "child.h"
#include "common.h"
#include "ipc_client.h"
#include "log.h"
#include "pa2345.h"
#include "stdbool.h"

#define MAX_PROCS 10

char* build_msg(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  int size = vsnprintf(NULL, 0, fmt, args);
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

static struct option long_opts[] = {{"mutexl", no_argument, NULL, 'm'}};

static char* short_opts = "p:m";

int main(int argc, char* argv[]) {
  int opt;
  uint16_t procs = 0;
  bool mutexl = false;

  while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (opt) {
      case 'p':
        procs = strtoul(optarg, NULL, 10);

        if (procs > MAX_PROCS) {
          fprintf(stderr, "%s: %s: %d\n", argv[0],
                  "number of processes can't be more than", MAX_PROCS);
          return EXIT_FAILURE;
        }

        if (procs == 0) {
          fprintf(stderr, "%s: %s\n", argv[0],
                  "number of processes have to be greater than 0");
          return EXIT_FAILURE;
        }

        break;
      case 'm':
        mutexl = true;
        break;
      default:
        fprintf(stderr, "usage: main -p PROCS_NUM [--mutexl|-m]\n");
        return EXIT_SUCCESS;
    }
  }

  FILE* eventsf = fopen(events_log, "a");
  if (eventsf == NULL) {
    fprintf(stderr, "%s: %s: %s\n", argv[0], "fopen", strerror(errno));
    return EXIT_FAILURE;
  }
  Log* log = new_log(2, stdout, eventsf);

  Store* store = new_store(procs + 1);
  if (store == NULL) {
    fprintf(stderr, "%s: %s\n", argv[0], "new_store");
    return EXIT_FAILURE;
  }

  for (local_id id = PARENT_ID + 1; id <= procs; id++) {
    pid_t pid = fork();
    if (pid == 0) {
      free_store(store);
      free_log(log);
      fclose(eventsf);

      return EXIT_SUCCESS;
    } else if (pid < 0) {
      fprintf(stderr, "%s: %s: %s\n", argv[0], "fork", strerror(errno));
      return EXIT_FAILURE;
    }
  }

  IpcClient client = {PARENT_ID, store};
  init_client(&client);

  ReceiveAllError err;
  if ((err = receive_from_all(&client, STARTED)) != RCV_ALL_OK) {
    fprintf(stderr, "error: process %1d: %s\n", client.id,
            str_receive_error(err));
    return EXIT_FAILURE;
  }
  logfmt(log, log_received_all_started_fmt, get_lamport_time(), client.id);

  increment_lamprot_time();
  MessageHeader header = {MESSAGE_MAGIC, 0, STOP, get_lamport_time()};
  Message msg = {header};

  if (send_multicast(&client, &msg) != 0) {
    fprintf(stderr, "error: process %1d: %s: %s\n", client.id, "send_multicast",
            "STOP");
    return EXIT_FAILURE;
  }

  if ((err = receive_from_all(&client, DONE)) != RCV_ALL_OK) {
    fprintf(stderr, "error: process %1d: %s\n", client.id,
            str_receive_error(err));
    return EXIT_FAILURE;
  }
  logfmt(log, log_received_all_done_fmt, get_lamport_time(), client.id);

  AllHistory history;
  history.s_history_len = procs;

  Message rcv_msg;
  for (local_id id = PARENT_ID + 1; id <= procs; id++) {
    if (receive(&client, id, &rcv_msg) != 0) {
      fprintf(stderr, "error: process %1d while receiving from %1d\n",
              client.id, id);
      return EXIT_FAILURE;
    }

    MessageHeader hdr = rcv_msg.s_header;
    if (hdr.s_magic != MESSAGE_MAGIC && hdr.s_type != BALANCE_HISTORY) {
      fprintf(stderr,
              "error: process %1d while receiving balance history from %1d\n",
              client.id, id);
      return EXIT_FAILURE;
    }
    align_lamport_time(hdr.s_local_time);

    memcpy(&history.s_history[id - 1], rcv_msg.s_payload, hdr.s_payload_len);
  }

  print_history(&history);

  free_store(store);
  free_log(log);
  fclose(eventsf);

  int status = 0;
  for (local_id i = PARENT_ID + 1; i <= procs; i++) {
    int ok;
    wait(&ok);
    status |= ok;
  }
  return status;
}
