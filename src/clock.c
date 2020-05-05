#include "clock.h"

static timestamp_t now = 0;

timestamp_t get_lamport_time() { return now; }

void increment_lamprot_time() { now++; }

void align_lamport_time(timestamp_t rcvd) {
  if (now < rcvd) {
    now = rcvd;
  }
  increment_lamprot_time();
}
