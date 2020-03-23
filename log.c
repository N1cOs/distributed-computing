#include "log.h"

Log *new_log(uint32_t fn, ...) {
  Log *log = malloc(sizeof(Log));
  log->fn = fn;
  log->fs = malloc(sizeof(FILE *) * fn);

  va_list args;
  va_start(args, fn);
  for (uint8_t i = 0; i < fn; i++) {
    log->fs[i] = va_arg(args, FILE *);
  }
  va_end(args);

  return log;
}

void free_log(Log *log) {
  free(log->fs);
  free(log);
}

int flog(Log *log, const char *const fmt, ...) {
  va_list args;
  for (uint8_t i = 0; i < log->fn; i++) {
    FILE *f = log->fs[i];
    va_start(args, fmt);

    if (vfprintf(f, fmt, args) < 0) {
      va_end(args);
      return -1;
    }
    va_end(args);
  }
  return 0;
}
