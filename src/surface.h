#ifndef OMABAR_SURFACE_H
#define OMABAR_SURFACE_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdint.h>

#include "layer.h"

struct surface {
    struct layer *layer;
    CGContextRef context;
    uint32_t id;
    uint32_t wid;
};

struct surface *surface_create(uint32_t wid, int width, int height);
void surface_destroy(struct surface *surface);
void surface_resize(struct surface *surface, int width, int height);
void surface_flush(struct surface *surface);

#endif