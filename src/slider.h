#ifndef OMABAR_SLIDER_H
#define OMABAR_SLIDER_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>

#include "color.h"

struct slider {
    double value;
    double min;
    double max;
    int width;
    int height;
    int line_width;
    struct color foreground_color;
    struct color slider_color;
    int done;
};

struct slider *slider_create(void);
void slider_destroy(struct slider *slider);
void slider_set_value(struct slider *slider, double value);
void slider_draw(struct slider *slider, struct CGContext *ctx, CGRect frame);

#endif