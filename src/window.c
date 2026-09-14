#include "window.h"
#include "bar.h"
#include "bar_manager.h"
#include "misc/extern.h"
#include "misc/helpers.h"

uint32_t g_connection = 0;

void window_freeze(void) {
    SLSDisableUpdate(g_connection);
}

void window_unfreeze(void) {
    SLSReenableUpdate(g_connection);
}

struct window *window_open(struct bar *bar, int width, int height) {
    (void)bar;
    return NULL;
}

void window_close(struct window *window) {
    if (!window) return;
    if (window->surface) {
        surface_destroy(window->surface);
        window->surface = NULL;
    }
    if (window->context)
        CGContextRelease(window->context);
    SLSReleaseWindow(g_connection, window->id);
    free(window);
}

void window_order(struct window *window, int order, int relative_to) {
    if (!window) return;
    SLSOrderWindow(g_connection, window->id, order, (unsigned int)relative_to);
}

void window_set_frame(struct window *window, int x, int y, int width, int height) {
    if (!window) return;
    window->frame = (CGRect){ {x, y}, {width, height} };
    window->origin = (CGPoint){x, y};
    SLSSetWindowOrigin(g_connection, window->id, (float)x, (float)y);

    CGRect rect = { {x, y}, {width, height} };
    CFTypeRef region = NULL;
    CGSNewRegionWithRect(&rect, &region);
    if (region) {
        SLSSetWindowShape(g_connection, window->id, 0, 0, region);
        CFRelease(region);
    }

    if (window->surface)
        surface_resize(window->surface, width, height);
}

void window_set_origin(struct window *window, int x, int y) {
    if (!window) return;
    window->origin = (CGPoint){x, y};
    SLSSetWindowOrigin(g_connection, window->id, x, y);
}

uint32_t window_connection(void) {
    if (g_connection == 0)
        g_connection = SLSMainConnectionID();
    return g_connection;
}