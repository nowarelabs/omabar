#ifndef OMABAR_EVENT_H
#define OMABAR_EVENT_H

#include <stdint.h>
#include <stdbool.h>

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
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_MOUSE_DRAGGED,
    EVENT_MOUSE_ENTERED,
    EVENT_MOUSE_EXITED,
    EVENT_MOUSE_SCROLLED,
    EVENT_SCROLL_TICK,
    EVENT_ANIMATION_TICK,
    EVENT_HOTLOAD,
    EVENT_MENU_BAR_HIDDEN_CHANGED,
    EVENT_CUSTOM,          /* plugin / external events */
    EVENT_DAEMON_MESSAGE,  /* IPC command from the CLI tool */
    EVENT_COUNT
};

struct event {
    enum event_type type;
    uint64_t arg1;
    uint64_t arg2;
    void *data;
};

typedef void (*event_handler_fn)(const struct event *event);

void event_init(void);
void event_post(struct event *event);
void event_execute(const struct event *event);

extern event_handler_fn g_event_handler[EVENT_COUNT];

#endif