#ifndef OMABAR_GRAPH_H
#define OMABAR_GRAPH_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>
#include <stdint.h>

#include "color.h"

#define GRAPH_MAX_POINTS 256

struct graph {
    struct color fill_color;
    struct color line_color;
    float *data;
    int count;
    int width;
    int height;
};

struct graph *graph_create(int width, int height);
void graph_destroy(struct graph *graph);
void graph_set_fill_color(struct graph *graph, struct color color);
void graph_set_line_color(struct graph *graph, struct color color);
void graph_push_value(struct graph *graph, float value);
void graph_draw(struct graph *graph, struct CGContext *ctx, CGRect frame);

#endif