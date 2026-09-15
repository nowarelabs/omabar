#include "event.h"
#include "window.h"
#include <dispatch/dispatch.h>
#include <pthread.h>

event_handler_fn g_event_handler[EVENT_COUNT];

static void default_handler(const struct event *event) {
    (void)event;
}

void event_init(void) {
    for (int i = 0; i < EVENT_COUNT; i++)
        g_event_handler[i] = default_handler;
}

void event_post(struct event *event) {
    if (!event) return;
    if (dispatch_get_main_queue() == NULL ||
        pthread_main_np() == 0)
        dispatch_sync(dispatch_get_main_queue(), ^{
            event_execute(event);
        });
    else
        event_execute(event);
}

void event_execute(const struct event *event) {
    if (event && event->type < EVENT_COUNT)
        g_event_handler[event->type](event);
    windows_unfreeze();
}
