#ifndef OMABAR_MOUSE_H
#define OMABAR_MOUSE_H

#include <stdint.h>
#include <CoreGraphics/CoreGraphics.h>

struct mouse_event {
    uint32_t type;
    uint32_t button;
    uint32_t modifier;
    CGPoint location;
    int scroll_delta;
};

typedef void (*mouse_handler_fn)(struct mouse_event *mouse_event);

void mouse_begin(mouse_handler_fn handler);
void mouse_end(void);
void mouse_set_window(uint32_t window_id);

#endif