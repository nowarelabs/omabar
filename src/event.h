#ifndef OMABAR_EVENT_H
#define OMABAR_EVENT_H

#include "event_loop.h"

typedef void (*event_handler_fn)(struct event *event);

void event_init(void);
void event_post(struct event *event);
void event_execute(struct event *event);

extern event_handler_fn g_event_handler[EVENT_COUNT];

#endif
