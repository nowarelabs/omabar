#include "surface.h"
#include "context.h"
#include "window.h"
#include "misc/extern.h"
#include <stdlib.h>

struct surface *surface_create(uint32_t wid, int width, int height) {
    struct surface *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->wid = wid;

    if (SLSAddSurface(window_connection(), wid, &s->id) != 0 || s->id == 0) {
        free(s);
        return NULL;
    }

    s->layer = layer_create(window_connection());
    if (!s->layer) {
        SLSRemoveSurface(window_connection(), wid, s->id);
        free(s);
        return NULL;
    }

    SLSBindSurface(window_connection(), wid, s->id, 0, 0, s->layer->caid);
    s->context = context_create(width, height);
    return s;
}

void surface_destroy(struct surface *surface) {
    if (!surface) return;
    SLSUnbindSurface(window_connection(), surface->wid, surface->id);
    if (surface->context)
        CGContextRelease(surface->context);
    if (surface->layer)
        layer_destroy(surface->layer);
    SLSRemoveSurface(window_connection(), surface->wid, surface->id);
    free(surface);
}

void surface_resize(struct surface *surface, int width, int height) {
    if (!surface) return;
    CGContextRelease(surface->context);
    surface->context = context_create(width, height);
    SLSSetSurfaceBounds(window_connection(), surface->wid, surface->id,
                        CGRectMake(0, 0, width, height));
}

void surface_flush(struct surface *surface) {
    if (!surface || !surface->context || !surface->layer)
        return;

    CGImageRef image = CGBitmapContextCreateImage(surface->context);
    if (!image) return;

    layer_set_contents(surface->layer, image);
    CGImageRelease(image);
    SLSFlushSurface(window_connection(), surface->wid, surface->id,
                    (int)surface->layer->caid);
}