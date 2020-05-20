#include "priority_queue.h"

bool equal_tuples(Tuple t1, Tuple t2) {
  return t1.proc == t2.proc && t1.time == t2.time;
}

PriorityQueue *new_prioriy_queue(uint16_t capacity) {
  PriorityQueue *queue = malloc(sizeof(PriorityQueue));
  if (queue == NULL) {
    return NULL;
  }

  Tuple *head = malloc(sizeof(Tuple) * capacity);
  if (head == NULL) {
    return NULL;
  }

  queue->next = 0;
  queue->capacity = capacity;
  queue->head = head;

  return queue;
}

void free_priority_queue(PriorityQueue *queue) {
  free(queue->head);
  free(queue);
}

static void _shift_right(Tuple *queue, uint16_t start, uint16_t end) {
  Tuple new;
  Tuple prev = queue[start];
  for (uint16_t i = start + 1; i < end; i++) {
    new = prev;
    prev = queue[i];
    queue[i] = new;
  }
}

static void _shift_left(Tuple *queue, uint16_t start, uint16_t end) {
  Tuple new;
  Tuple prev = queue[end - 1];
  for (int i = end - 2; i >= start; i--) {
    new = prev;
    prev = queue[i];
    queue[i] = new;
  }
}

void add(PriorityQueue *queue, Tuple new) {
  if (queue->capacity == queue->next) {
    uint16_t new_cap = queue->capacity * 2;
    Tuple *head = realloc(queue->head, sizeof(Tuple) * new_cap);

    queue->head = head;
    queue->capacity = new_cap;
  }

  Tuple last = new;
  for (uint16_t i = 0; i < queue->next; i++) {
    Tuple cur = queue->head[i];
    if (new.time < cur.time || (new.time == cur.time &&new.proc < cur.proc)) {
      last = queue->head[queue->next - 1];
      _shift_right(queue->head, i, queue->next);

      queue->head[i] = new;
      break;
    }
  }

  queue->head[queue->next] = last;
  queue->next++;
}

Tuple pop(PriorityQueue *queue) {
  Tuple head = queue->head[0];
  _shift_left(queue->head, 0, queue->next);
  queue->next--;
  return head;
}

Tuple top(PriorityQueue *queue) { return queue->head[0]; }
