#include "shadow.h"
#include "misc/helpers.h"
#include <math.h>

void shadow_init(struct shadow *shadow) {
    if (!shadow) return;
    shadow->enabled = false;
    shadow->angle = 30;
    shadow->distance = 5;
    shadow->color = color_from_hex(0x99000000);
    shadow->offset = CGPointMake(
        (float)shadow->distance * cos(shadow->angle * M_PI / 180.0),
        (float)-shadow->distance * sin(shadow->angle * M_PI / 180.0));
}

static void shadow_recompute_offset(struct shadow *shadow) {
    shadow->offset = CGPointMake(
        (float)shadow->distance * cos(shadow->angle * M_PI / 180.0),
        (float)-shadow->distance * sin(shadow->angle * M_PI / 180.0));
}

void shadow_set_enabled(struct shadow *shadow, int enabled) {
    if (shadow) shadow->enabled = enabled != 0;
}

void shadow_set_angle(struct shadow *shadow, double angle) {
    if (!shadow) return;
    shadow->angle = angle;
    shadow_recompute_offset(shadow);
}

void shadow_set_distance(struct shadow *shadow, double distance) {
    if (!shadow) return;
    shadow->distance = distance;
    shadow_recompute_offset(shadow);
}

void shadow_set_color(struct shadow *shadow, struct color color) {
    if (!shadow) return;
    shadow->color = color;
    shadow->enabled = true;
}

CGRect shadow_get_bounds(struct shadow *shadow, CGRect reference_bounds) {
    return CGRectMake(reference_bounds.origin.x + shadow->offset.x,
                      reference_bounds.origin.y + shadow->offset.y,
                      reference_bounds.size.width,
                      reference_bounds.size.height);
}

void shadow_draw(struct shadow *shadow, CGContextRef ctx, CGPathRef path) {
    if (!shadow || !shadow->enabled || !ctx || !path) return;
    CGContextSaveGState(ctx);
    CGContextSetRGBFillColor(ctx,
                             shadow->color.r,
                             shadow->color.g,
                             shadow->color.b,
                             shadow->color.a);
    CGContextAddPath(ctx, path);
    CGContextFillPath(ctx);
    CGContextRestoreGState(ctx);
}

void shadow_draw_cg(struct shadow *shadow, CGContextRef ctx, CGPathRef path) {
    if (!shadow || !shadow->enabled || !ctx || !path) return;

    CGContextSaveGState(ctx);
    CGSize offset = CGSizeMake(shadow->offset.x, shadow->offset.y);
    CGContextSetShadowWithColor(ctx,
                                offset,
                                (CGFloat)shadow->distance,
                                cgcolor_from_color(shadow->color));
    CGContextSetRGBFillColor(ctx, 0.0f, 0.0f, 0.0f, 1.0f);
    CGContextAddPath(ctx, path);
    CGContextFillPath(ctx);
    CGContextRestoreGState(ctx);
}