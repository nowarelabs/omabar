#ifndef OMABAR_BACKGROUND_H
#define OMABAR_BACKGROUND_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>

#include "color.h"
#include "image.h"
#include "shadow.h"

struct background {
    struct color color;
    struct color border_color;
    struct image *image;
    struct shadow shadow;

    bool has_shadow;
    int corner_radius;
    int border_width;
    int padding_left;
    int padding_right;
    int padding_top;
    int padding_bottom;
    int clip;
    int y_offset;

    unsigned int type:1; /* 0=rect, 1=circle */
};

void background_init(struct background *bg);
void background_destroy(struct background *bg);
void background_draw(struct background *bg, CGContextRef ctx, CGRect bounds);

#endif