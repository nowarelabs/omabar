#ifndef OMABAR_SHADOW_H
#define OMABAR_SHADOW_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>

#include "color.h"

struct shadow {
    struct color color;
    double angle;
    double distance;
    CGPoint offset;
    bool enabled;
};

void shadow_init(struct shadow *shadow);
void shadow_set_enabled(struct shadow *shadow, int enabled);
void shadow_set_angle(struct shadow *shadow, double angle);
void shadow_set_distance(struct shadow *shadow, double distance);
void shadow_set_color(struct shadow *shadow, struct color color);
CGRect shadow_get_bounds(struct shadow *shadow, CGRect reference_bounds);
void shadow_draw(struct shadow *shadow, CGContextRef ctx, CGPathRef path);
void shadow_draw_cg(struct shadow *shadow, CGContextRef ctx, CGPathRef path);

#endif