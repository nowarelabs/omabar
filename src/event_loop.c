#include "event_loop.h"
#include <stdlib.h>
#include <dispatch/dispatch.h>

static void event_loop_run(void *data) {
    struct event_loop *el = data;
    while (el->running || atomic_load(&el->head) != atomic_load(&el->tail)) {
        if (atomic_load(&el->head) == atomic_load(&el->tail)) {
            /* spin or yield — for now, brief sleep */
            usleep(1000);
            continue;
        }

        uint64_t tail = atomic_load(&el->tail);
        struct event *e = el->queue[tail % EVENT_QUEUE_CAPACITY];
        atomic_store(&el->tail, tail + 1);

        if (e) {
            /* dispatch to main thread for state mutation */
            dispatch_async(dispatch_get_main_queue(), ^{
                /* placeholder: actual event_execute goes here */
                free(e);
            });
        }
    }
}

void event_loop_init(struct event_loop *el) {
    memset(el, 0, sizeof(*el));
    atomic_store(&el->head, 0);
    atomic_store(&el->tail, 0);
    pthread_mutex_init(&el->lock, NULL);
}

void event_loop_begin(struct event_loop *el) {
    el->running = 1;
    pthread_create(&el->worker, NULL,
                   (void *(*)(void *))event_loop_run, el);
}

void event_loop_destroy(struct event_loop *el) {
    el->running = 0;
    pthread_join(el->worker, NULL);
    pthread_mutex_destroy(&el->lock);
    struct event *e;
    while ((e = el->queue[atomic_load(&el->tail) % EVENT_QUEUE_CAPACITY]) != NULL) {
        free(e);
        atomic_fetch_add(&el->tail, 1);
    }
}

int event_loop_post(struct event_loop *el, struct event *event) {
    uint64_t head = atomic_load(&el->head);
    if (head - atomic_load(&el->tail) >= EVENT_QUEUE_CAPACITY)
        return -1;

    el->queue[head % EVENT_QUEUE_CAPACITY] = event;
    atomic_store(&el->head, head + 1);
    return 0;
}
