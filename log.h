#ifndef __IFMO_DISTRIBUTED_CLASS_LOG__H
#define __IFMO_DISTRIBUTED_CLASS_LOG__H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint32_t fn;
  FILE **fs;
} Log;

Log *new_log(uint32_t fn, ...);

int flog(Log *log, const char *const fmt, ...);

void free_log(Log *log);

#endif  // __IFMO_DISTRIBUTED_CLASS_LOG__H
