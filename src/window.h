#ifndef OMABAR_WINDOW_H
#define OMABAR_WINDOW_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdint.h>
#include <stdbool.h>

#include "surface.h"

#define W_ABOVE 1
#define W_OUT   0
#define W_BELOW -1

#define kCGSExposeFadeTagBit         (1ULL << 1)
#define kCGSPreventsActivationTagBit (1ULL << 16)

struct bar;

struct window {
    uint32_t id;
    CGContextRef context;
    struct surface *surface;
    struct window *parent;
    int order_mode;
    bool needs_move;
    bool needs_resize;

    CGRect frame;
    CGPoint origin;
    int refc;
};

extern CGPoint g_nirvana;
extern CFTypeRef g_transaction;

void window_init(struct window *window);
void window_clear(struct window *window);

struct window *window_open(struct bar *bar, int width, int height);
void window_close(struct window *window);
void window_destroy(struct window *window);
void window_flush(struct window *window);

void window_move(struct window *window, CGPoint point);
void window_set_frame(struct window *window, int x, int y, int width, int height);
void window_set_origin(struct window *window, int x, int y);
bool window_apply_frame(struct window *window, bool forced);
void window_send_to_space(struct window *window, uint64_t dsid);

void window_set_blur_radius(struct window *window, uint32_t radius);
void window_disable_shadow(struct window *window);
void window_set_level(struct window *window, uint32_t level);
void window_order(struct window *window, int order, int relative_to);
void window_assign_mouse_tracking_area(struct window *window, CGRect rect);

void windows_freeze(void);
void windows_unfreeze(void);

uint32_t window_connection(void);

#endif