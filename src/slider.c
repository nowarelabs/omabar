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
    return s;
}

void slider_destroy(struct slider *slider) {
    if (slider) free(slider);
}

void slider_set_value(struct slider *slider, double value) {
    if (!slider) return;
    if (value < slider->min) value = slider->min;
    if (value > slider->max) value = slider->max;
    slider->value = value;
}

void slider_draw(struct slider *slider, struct CGContext *ctx, CGRect frame) {
    (void)slider; (void)ctx; (void)frame;
}