#ifndef __IFMO_DISTRIBUTED_CLASS_CLOCK__H
#define __IFMO_DISTRIBUTED_CLASS_CLOCK__H

#include "banking.h"

void increment_lamprot_time();

void align_lamport_time(timestamp_t rcvd);

#endif  // __IFMO_DISTRIBUTED_CLASS_CLOCK__H
