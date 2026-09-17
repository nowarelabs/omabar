#ifndef OMABAR_GRAPH_H
#define OMABAR_GRAPH_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>
#include <stdint.h>

#include "color.h"

#define GRAPH_MAX_POINTS 256

struct bar_item;

/* Optional live-data hook: pushed from the animation tick ("scroll.tick")
   so a graph scrolls continuously. */
typedef bool (*graph_data_source_t)(struct bar_item *item);

struct graph {
    struct color fill_color;
    struct color line_color;
    float *data;
    int count;
    int max_points;
    float width;
    float height;
    float line_width;
    float min_value;
    float max_value;
    graph_data_source_t data_source;
    bool scroll_fill;   /* fill the whole plot area (vs only under the line) */
};

struct graph *graph_create(float width, float height);
void graph_destroy(struct graph *graph);
void graph_set_fill_color(struct graph *graph, struct color color);
void graph_set_line_color(struct graph *graph, struct color color);
void graph_set_line_width(struct graph *graph, float line_width);
void graph_set_range(struct graph *graph, float min_value, float max_value);
void graph_set_max_points(struct graph *graph, int max_points);
void graph_set_data_source(struct graph *graph, graph_data_source_t source);
void graph_push_value(struct graph *graph, float value);
void graph_clear(struct graph *graph);
void graph_draw(struct graph *graph, CGContextRef ctx, CGRect frame);

#endif