#ifndef OMABAR_EVENT_LOOP_H
#define OMABAR_EVENT_LOOP_H

#include "event.h"
#include <pthread.h>

#define EVENT_QUEUE_CAPACITY 1024

struct event_loop {
    struct event *slot;          /* mmap'd ring buffer (event memory pool) */
    uint64_t *seq;               /* mmap'd per-slot sequence counters */
    uint64_t head;               /* next logical slot for producers (atomic) */
    uint64_t tail;               /* next logical slot for the consumer (atomic) */
    pthread_mutex_t lock;        /* held only during teardown */
    pthread_t worker;            /* dedicated consumer thread */
    volatile int running;
};

void event_loop_init(struct event_loop *el);
void event_loop_begin(struct event_loop *el);
void event_loop_destroy(struct event_loop *el);
int  event_loop_post(struct event_loop *el, struct event *event);

#endif