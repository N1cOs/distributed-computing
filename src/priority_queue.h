#ifndef __IFMO_DISTRIBUTED_CLASS_PRIORITY_QUEUE__H
#define __IFMO_DISTRIBUTED_CLASS_PRIORITY_QUEUE__H

#include "ipc.h"
#include "stdbool.h"
#include "stdlib.h"

typedef struct {
  timestamp_t time;
  local_id proc;
} Tuple;

typedef struct {
  Tuple *head;
  uint16_t next;
  uint16_t capacity;
} PriorityQueue;

bool equal_tuples(Tuple t1, Tuple t2);

PriorityQueue *new_prioriy_queue(uint16_t capacity);

void free_priority_queue(PriorityQueue *queue);

void add(PriorityQueue *queue, Tuple value);

Tuple pop(PriorityQueue *queue);

Tuple top(PriorityQueue *queue);

#endif  // __IFMO_DISTRIBUTED_CLASS_PRIORITY_QUEUE__H
