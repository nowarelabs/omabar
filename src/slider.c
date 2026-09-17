#include "slider.h"
#include "misc/helpers.h"
#include <stdlib.h>

struct slider *slider_create(void) {
    struct slider *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->min = 0.0;
    s->max = 100.0;
    s->value = 0.0;
    s->width = 100;
    s->height = 6;
    s->line_width = 1;
    s->foreground_color = color_from_hex(0xff40a0ff);
    s->slider_color = color_from_hex(0xffffffff);
    s->done = 0;
    s->is_dragged = 0;
    return s;
}

void slider_destroy(struct slider *slider) {
    if (slider) free(slider);
}

void slider_setup(struct slider *slider, int width, int height) {
    if (!slider) return;
    slider->width = width;
    slider->height = height;
}

void slider_set_value(struct slider *slider, double value) {
    if (!slider) return;
    if (value < slider->min) value = slider->min;
    if (value > slider->max) value = slider->max;
    slider->value = value;
}

void slider_set_range(struct slider *slider, double value, double min, double max) {
    if (!slider) return;
    slider->min = min;
    slider->max = max;
    slider_set_value(slider, value);
}

void slider_cancel_drag(struct slider *slider) {
    if (!slider) return;
    slider->is_dragged = false;
    slider->done = true;
}

double slider_value_for_point(struct slider *slider, CGPoint point, CGRect frame) {
    if (!slider) return slider ? slider->value : 0;
    float delta = point.x - frame.origin.x;
    if (delta < 0) delta = 0;
    float fraction = delta / frame.size.width;
    if (fraction > 1.0f) fraction = 1.0f;
    return slider->min + fraction * (slider->max - slider->min);
}

bool slider_handle_drag(struct slider *slider, CGPoint point, CGRect frame) {
    if (!slider) return false;
    double new_value = slider_value_for_point(slider, point, frame);
    if (fabs(new_value - slider->value) < 0.0000001 && slider->is_dragged)
        return false;
    slider->value = new_value;
    slider->is_dragged = true;
    return true;
}

bool slider_hit_test(CGRect frame, CGPoint point) {
    return CGRectContainsPoint(CGRectInset(frame, -15, -15), point);
}

static void draw_track(CGContextRef ctx, CGRect rect, float radius,
                       struct color c) {
    CGMutablePathRef p = CGPathCreateMutable();
    CGPathAddRoundedRect(p, NULL, rect, radius, radius);
    CGContextAddPath(ctx, p);
    CGContextFillPath(ctx);
    CGPathRelease(p);
}

void slider_draw(struct slider *slider, CGContextRef ctx, CGRect frame) {
    if (!slider || !ctx) return;

    CGContextSaveGState(ctx);

    float radius = (float)slider->height / 2.0f;
    if (radius > frame.size.height / 2.0f)
        radius = (float)frame.size.height / 2.0f;
    if (radius < 1.0f) radius = 1.0f;

    CGContextSetRGBFillColor(ctx,
                             slider->slider_color.r,
                             slider->slider_color.g,
                             slider->slider_color.b,
                             slider->slider_color.a * 0.35f);
    draw_track(ctx, frame, radius, slider->slider_color);

    float fraction = frame.size.width > 0
        ? (float)((slider->value - slider->min)
                  / (slider->max - slider->min))
        : 0.0f;
    if (fraction < 0) fraction = 0;
    if (fraction > 1) fraction = 1;

    CGRect fill = (CGRect){ { frame.origin.x, frame.origin.y },
                            { frame.size.width * fraction, frame.size.height } };
    if (fill.size.width > 0) {
        CGContextSetRGBFillColor(ctx,
                                 slider->foreground_color.r,
                                 slider->foreground_color.g,
                                 slider->foreground_color.b,
                                 slider->foreground_color.a);
        draw_track(ctx, fill, radius, slider->foreground_color);
    }

    float knob_x = frame.origin.x + frame.size.width * fraction;
    float knob_y = frame.origin.y + frame.size.height / 2.0f;
    float knob_r = radius + slider->line_width;
    CGContextSetRGBFillColor(ctx,
                             slider->slider_color.r,
                             slider->slider_color.g,
                             slider->slider_color.b,
                             slider->slider_color.a);
    CGContextFillEllipseInRect(ctx,
                               CGRectMake(knob_x - knob_r,
                                          knob_y - knob_r,
                                          knob_r * 2,
                                          knob_r * 2));

    CGContextRestoreGState(ctx);
}