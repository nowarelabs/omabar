#ifndef OMABAR_EVENT_LOOP_H
#define OMABAR_EVENT_LOOP_H

#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#define EVENT_QUEUE_CAPACITY 1024

enum event_type {
    EVENT_MACH_MESSAGE = 0,
    EVENT_DISPLAY_ADDED,
    EVENT_DISPLAY_REMOVED,
    EVENT_DISPLAY_MOVED,
    EVENT_SPACE_CHANGED,
    EVENT_WINDOW_FOCUSED,
    EVENT_APP_FRONT_SWITCHED,
    EVENT_APP_LAUNCHED,
    EVENT_APP_TERMINATED,
    EVENT_VOLUME_CHANGED,
    EVENT_POWER_CHANGED,
    EVENT_WIFI_CHANGED,
    EVENT_MEDIA_CHANGED,
    EVENT_BRIGHTNESS_CHANGED,
    EVENT_MOUSE_CLICKED,
    EVENT_MOUSE_ENTERED,
    EVENT_MOUSE_EXITED,
    EVENT_MOUSE_SCROLLED,
    EVENT_SCROLL_TICK,
    EVENT_ANIMATION_TICK,
    EVENT_HOTLOAD,
    EVENT_CUSTOM,
    EVENT_DAEMON_MESSAGE,
    EVENT_COUNT
};

struct event {
    enum event_type type;
    uint64_t arg1;
    uint64_t arg2;
    void *data;
};

struct event_loop {
    struct event *queue[EVENT_QUEUE_CAPACITY];
    _Atomic uint64_t head;
    _Atomic uint64_t tail;
    pthread_mutex_t lock;
    pthread_t worker;
    volatile int running;
};

void event_loop_init(struct event_loop *el);
void event_loop_begin(struct event_loop *el);
void event_loop_destroy(struct event_loop *el);
int  event_loop_post(struct event_loop *el, struct event *event);

#endif
