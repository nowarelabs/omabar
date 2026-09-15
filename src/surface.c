#include "surface.h"
#include "context.h"
#include "window.h"
#include "misc/extern.h"
#include <stdlib.h>

struct surface *surface_create(uint32_t wid, int width, int height) {
    struct surface *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->wid = wid;

    s->layer = layer_create(window_connection());
    if (!s->layer) { free(s); return NULL; }

    if (SLSAddSurface(window_connection(), wid, &s->id) != 0 || s->id == 0) {
        layer_destroy(s->layer);
        free(s);
        return NULL;
    }

    SLSBindSurface(window_connection(), wid, s->id, 0x4, 0, s->layer->caid);

    CGRect frame = CGRectMake(0, 0, width, height);
    SLSSetSurfaceBounds(window_connection(), wid, s->id, frame);
    SLSSetSurfaceResolution(window_connection(), wid, s->id, 2.0);
    SLSSetSurfaceOpacity(window_connection(), wid, s->id, false);

    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    SLSSetSurfaceColorSpace(window_connection(), wid, s->id, color_space);
    CGColorSpaceRelease(color_space);

    SLSOrderSurface(window_connection(), wid, s->id, 1, 0);
    SLSFlushSurface(window_connection(), wid, s->id, 0);

    s->context = context_create(width, height);
    return s;
}

void surface_destroy(struct surface *surface) {
    if (!surface) return;
    if (surface->id)
        SLSRemoveSurface(window_connection(), surface->wid, surface->id);
    if (surface->layer)
        layer_destroy(surface->layer);
    if (surface->context)
        CGContextRelease(surface->context);
    free(surface);
}

void surface_resize(struct surface *surface, int width, int height) {
    if (!surface) return;
    if (surface->context)
        CGContextRelease(surface->context);
    surface->context = context_create(width, height);

    CGRect frame = CGRectMake(0, 0, width, height);
    SLSSetSurfaceBounds(window_connection(), surface->wid, surface->id, frame);
    layer_set_bounds(surface->layer, frame);
}

void surface_flush(struct surface *surface) {
    if (!surface || !surface->context || !surface->layer)
        return;

    CGImageRef image = CGBitmapContextCreateImage(surface->context);
    if (!image) return;

    layer_set_contents(surface->layer, image);
    SLSFlushSurface(window_connection(), surface->wid, surface->id, 0);
    CGImageRelease(image);
}