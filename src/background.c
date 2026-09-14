#include "background.h"
#include "misc/helpers.h"
#include <stdlib.h>

void background_init(struct background *bg) {
    memset(bg, 0, sizeof(*bg));
    bg->color = color_from_hex(0x00000000);
    bg->border_color = color_from_hex(0xffffffff);
    bg->corner_radius = 0;
    bg->border_width = 0;
}

void background_destroy(struct background *bg) {
    if (!bg) return;
    if (bg->image) {
        image_destroy(bg->image);
        free(bg->image);
        bg->image = NULL;
    }
}

static void background_draw_rect(struct background *bg, CGContextRef ctx, CGRect bounds) {
    CGMutablePathRef path = CGPathCreateMutable();
    CGPathAddRoundedRect(path, NULL, bounds, bg->corner_radius, bg->corner_radius);

    if (bg->color.a > 0) {
        CGContextSetFillColorWithColor(ctx, cgcolor_from_color(bg->color));
        CGContextAddPath(ctx, path);
        CGContextFillPath(ctx);
    }

    if (bg->border_width > 0 && bg->border_color.a > 0) {
        CGContextSetStrokeColorWithColor(ctx, cgcolor_from_color(bg->border_color));
        CGContextSetLineWidth(ctx, bg->border_width);
        CGContextAddPath(ctx, path);
        CGContextStrokePath(ctx);
    }

    CGPathRelease(path);
}

void background_draw(struct background *bg, struct CGContext *ctx, CGRect bounds) {
    (void)bg; (void)ctx; (void)bounds;
}