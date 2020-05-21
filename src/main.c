#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "child.h"
#include "common.h"
#include "string.h"

#define MAX_PROCS 10

static struct option long_opts[] = {{"mutexl", no_argument, NULL, 'm'},
                                    {0, 0, 0, 0}};

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

  Store* store = new_store(procs + 1);
  if (store == NULL) {
    fprintf(stderr, "%s: %s\n", argv[0], "new_store");
    return EXIT_FAILURE;
  }

  for (local_id id = PARENT_ID + 1; id <= procs; id++) {
    pid_t pid = fork();
    if (pid == 0) {
      Child* child = new_child(id, store);
      int status = exec_child(child, mutexl);

      free_child(child);
      free_store(store);
      fclose(eventsf);

      return status;
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

  if ((err = receive_from_all(&client, DONE)) != RCV_ALL_OK) {
    fprintf(stderr, "error: process %1d: %s\n", client.id,
            str_receive_error(err));
    return EXIT_FAILURE;
  }

  free_store(store);
  fclose(eventsf);

  int status = 0;
  for (local_id i = PARENT_ID + 1; i <= procs; i++) {
    int ok;
    wait(&ok);
    status |= ok;
  }
  return status;
}
