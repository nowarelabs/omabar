#ifndef OMABAR_WINDOW_H
#define OMABAR_WINDOW_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdint.h>
#include <stdbool.h>

#include "surface.h"

struct bar;

struct window {
    uint32_t id;
    CGContextRef context;
    struct surface *surface;
    CGRect frame;
    CGPoint origin;
    struct window *parent;
    int refc;
};

struct window *window_open(struct bar *bar, int width, int height);
uint32_t window_connection(void);
void window_close(struct window *window);
void window_order(struct window *window, int order, int relative_to);
void window_set_frame(struct window *window, int x, int y, int width, int height);
void window_set_origin(struct window *window, int x, int y);
void window_freeze(void);
void window_unfreeze(void);

#endif