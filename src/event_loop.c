#include "event_loop.h"
#include "event.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <dispatch/dispatch.h>

#define EVENT_QUEUE_MASK (EVENT_QUEUE_CAPACITY - 1)

/*
 * Lock-free bounded MPSC queue (Vyukov) implemented on top of an mmap'd
 * memory pool:
 *
 *   - producer threads claim a logical slot with
 *     __sync_bool_compare_and_swap on `head`, write the event by value,
 *     then publish it by bumping the slot's sequence counter.
 *   - a single consumer advances `tail`, waiting on the per-slot
 *     sequence counter, and hands the event off to the handlers.
 *
 * The mmap'd region holds both the ring of `struct event` values and
 * the parallel array of uint64_t sequence counters. Sequence numbers
 * make the protocol lock-free: a producer never touches a slot the
 * consumer has not finished reading, and vice versa.
 */

static void event_loop_yield(void) {
    sched_yield();
}

/*
 * Claim the next logical slot for a new event. Waits until the consumer
 * frees a slot (bounded queue backpressure): events are never dropped.
 * Callers must not hold locks while posting.
 */
static uint64_t event_loop_claim(struct event_loop *el) {
    for (;;) {
        uint64_t head = __atomic_load_n(&el->head, __ATOMIC_RELAXED);
        uint64_t tail = __atomic_load_n(&el->tail, __ATOMIC_RELAXED);
        if (head - tail < EVENT_QUEUE_CAPACITY
            && __sync_bool_compare_and_swap(&el->head, head, head + 1))
            return head;
        event_loop_yield();
    }
}

int event_loop_post(struct event_loop *el, struct event *event) {
    uint64_t pos = event_loop_claim(el);

    uint64_t slot_index = pos & EVENT_QUEUE_MASK;

    /* wait until the slot has been recycled by the consumer */
    while (__atomic_load_n(&el->seq[slot_index], __ATOMIC_ACQUIRE) != pos)
        event_loop_yield();

    el->slot[slot_index] = *event;

    /* publish: a consumer waiting for pos + 1 may now read the event */
    __atomic_store_n(&el->seq[slot_index], pos + 1, __ATOMIC_RELEASE);
    return 0;
}

/*
 * Try to pop the next event into `out`. Returns 1 on success, 0 when
 * the queue is momentarily empty.
 */
static int event_loop_pop(struct event_loop *el, struct event *out) {
    uint64_t pos = __atomic_load_n(&el->tail, __ATOMIC_RELAXED);
    uint64_t slot_index = pos & EVENT_QUEUE_MASK;

    if (__atomic_load_n(&el->seq[slot_index], __ATOMIC_ACQUIRE) != pos + 1)
        return 0;

    *out = el->slot[slot_index];

    /* recycle the slot for the producer writing at pos + EVENT_QUEUE_CAPACITY */
    __atomic_store_n(&el->seq[slot_index], pos + EVENT_QUEUE_CAPACITY,
                     __ATOMIC_RELEASE);
    __atomic_store_n(&el->tail, pos + 1, __ATOMIC_RELEASE);
    return 1;
}

#define EVENT_BATCH 64

static void *event_loop_run(void *data) {
    struct event_loop *el = data;
    struct event *batch = malloc(sizeof(struct event) * EVENT_BATCH);
    if (!batch) return NULL;

    while (el->running) {
        int count = 0;
        while (count < EVENT_BATCH && event_loop_pop(el, &batch[count]))
            count++;

        if (count > 0) {
            /* handlers mutate bar state, so run them on the main thread */
            dispatch_async(dispatch_get_main_queue(), ^{
                for (int i = 0; i < count; i++)
                    event_execute(&batch[i]);
                free(batch);
            });
            batch = malloc(sizeof(struct event) * EVENT_BATCH);
            if (!batch) break;
        } else {
            event_loop_yield();
        }
    }

    free(batch);
    return NULL;
}

void event_loop_init(struct event_loop *el) {
    memset(el, 0, sizeof(*el));

    size_t pool_size = sizeof(struct event) * EVENT_QUEUE_CAPACITY
                     + sizeof(uint64_t) * EVENT_QUEUE_CAPACITY;

    void *pool = mmap(NULL, pool_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANON, -1, 0);
    if (pool == MAP_FAILED) return;

    el->slot = pool;
    el->seq = (uint64_t *)((char *)pool
                            + sizeof(struct event) * EVENT_QUEUE_CAPACITY);

    for (int i = 0; i < EVENT_QUEUE_CAPACITY; i++)
        el->seq[i] = i;

    __atomic_store_n(&el->head, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&el->tail, 0, __ATOMIC_RELAXED);
    pthread_mutex_init(&el->lock, NULL);
}

void event_loop_begin(struct event_loop *el) {
    el->running = 1;
    pthread_create(&el->worker, NULL, event_loop_run, el);
}

void event_loop_destroy(struct event_loop *el) {
    el->running = 0;
    pthread_join(el->worker, NULL);
    pthread_mutex_destroy(&el->lock);

    size_t pool_size = sizeof(struct event) * EVENT_QUEUE_CAPACITY
                     + sizeof(uint64_t) * EVENT_QUEUE_CAPACITY;
    munmap(el->slot, pool_size);
    el->slot = NULL;
    el->seq = NULL;
}