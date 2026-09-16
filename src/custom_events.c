#include "custom_events.h"
#include "misc/helpers.h"
#include <stdlib.h>

void custom_events_init(struct custom_events *ce) {
    memset(ce, 0, sizeof(*ce));

    /* built-in events */
    const char *builtin[] = {
        "routine", "forced", "mouse.entered", "mouse.exited",
        "mouse.scrolled", "mouse.clicked", "volume_changed",
        "power_changed", "wifi_changed", "brightness_changed",
        "media_changed", "front_app_switched", "space_changed",
        "display_added", "display_removed", "display_moved",
        "window_focused", "scroll.tick", "animate", "daemon_message",
        "dnd",
        NULL
    };

    for (int i = 0; builtin[i]; i++)
        custom_events_register(ce, builtin[i], NULL);
}

void custom_events_destroy(struct custom_events *ce) {
    for (int i = 0; i < ce->count; i++) {
        free(ce->events[i].name);
        if (ce->events[i].notification)
            CFRelease(ce->events[i].notification);
    }
    memset(ce, 0, sizeof(*ce));
}

int custom_events_register(struct custom_events *ce, const char *name, const char *notification) {
    if (ce->count >= CUSTOM_EVENT_MAX) return -1;

    struct custom_event *e = &ce->events[ce->count];
    e->name = strdup(name);
    e->mask = 1ULL << ce->count;

    if (notification)
        e->notification = CFStringCreateWithCString(NULL, notification,
                                                    kCFStringEncodingUTF8);
    else
        e->notification = NULL;

    ce->count++;
    return 0;
}

uint64_t custom_events_get_mask(struct custom_events *ce, const char *name) {
    for (int i = 0; i < ce->count; i++) {
        if (strcmp(ce->events[i].name, name) == 0)
            return ce->events[i].mask;
    }
    return 0;
}

void custom_events_trigger(struct custom_events *ce, const char *name, void *data) {
    (void)ce; (void)name; (void)data;
}