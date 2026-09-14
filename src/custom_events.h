#ifndef OMABAR_CUSTOM_EVENTS_H
#define OMABAR_CUSTOM_EVENTS_H

#include <stdint.h>
#include <stdbool.h>
#include <CoreServices/CoreServices.h>

#define CUSTOM_EVENT_MAX 256

struct custom_event {
    char *name;
    CFStringRef notification;
    uint64_t mask;
};

struct custom_events {
    struct custom_event events[CUSTOM_EVENT_MAX];
    int count;
};

void custom_events_init(struct custom_events *ce);
void custom_events_destroy(struct custom_events *ce);
int  custom_events_register(struct custom_events *ce, const char *name, const char *notification);
uint64_t custom_events_get_mask(struct custom_events *ce, const char *name);
void custom_events_trigger(struct custom_events *ce, const char *name, void *data);

#endif