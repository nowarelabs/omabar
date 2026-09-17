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
    int is_dragged;
    int done;
};

struct slider *slider_create(void);
void slider_destroy(struct slider *slider);
void slider_setup(struct slider *slider, int width, int height);
void slider_set_value(struct slider *slider, double value);
void slider_set_range(struct slider *slider, double value, double min, double max);
void slider_cancel_drag(struct slider *slider);
double slider_value_for_point(struct slider *slider, CGPoint point, CGRect frame);
bool slider_handle_drag(struct slider *slider, CGPoint point, CGRect frame);
bool slider_hit_test(CGRect frame, CGPoint point);
void slider_draw(struct slider *slider, CGContextRef ctx, CGRect frame);

#endif