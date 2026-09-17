#include "graph.h"
#include "misc/helpers.h"
#include <stdlib.h>
#include <string.h>

struct graph *graph_create(float width, float height) {
    struct graph *g = calloc(1, sizeof(*g));
    if (!g) return NULL;
    g->width = width;
    g->height = height;
    g->fill_color = color_from_hex(0x00000000);
    g->line_color = color_from_hex(0xffffffff);
    g->line_width = 1.0f;
    g->min_value = 0.0f;
    g->max_value = 1.0f;
    g->max_points = GRAPH_MAX_POINTS;
    g->data = calloc((size_t)g->max_points, sizeof(float));
    g->count = 0;
    return g;
}

void graph_destroy(struct graph *graph) {
    if (!graph) return;
    free(graph->data);
    free(graph);
}

void graph_set_fill_color(struct graph *graph, struct color color) {
    if (graph) graph->fill_color = color;
}

void graph_set_line_color(struct graph *graph, struct color color) {
    if (graph) graph->line_color = color;
}

void graph_set_line_width(struct graph *graph, float line_width) {
    if (graph && line_width > 0.0f) graph->line_width = line_width;
}

void graph_set_range(struct graph *graph, float min_value, float max_value) {
    if (!graph || max_value <= min_value) return;
    graph->min_value = min_value;
    graph->max_value = max_value;
}

void graph_set_max_points(struct graph *graph, int max_points) {
    if (!graph || max_points < 2) return;
    if (max_points == graph->max_points) return;

    float *new_data = calloc((size_t)max_points, sizeof(float));
    if (!new_data) return;

    int keep = graph->count < max_points ? graph->count : max_points;
    int offset = graph->count - keep;
    if (graph->data && keep > 0)
        memcpy(new_data, graph->data + offset, (size_t)keep * sizeof(float));

    free(graph->data);
    graph->data = new_data;
    graph->count = keep;
    graph->max_points = max_points;
}

void graph_set_data_source(struct graph *graph, graph_data_source_t source) {
    if (graph) graph->data_source = source;
}

void graph_push_value(struct graph *graph, float value) {
    if (!graph) return;
    if (graph->count >= graph->max_points) {
        memmove(graph->data, graph->data + 1,
                (size_t)(graph->max_points - 1) * sizeof(float));
        graph->count = graph->max_points - 1;
    }
    graph->data[graph->count++] = value;
}

void graph_clear(struct graph *graph) {
    if (!graph) return;
    graph->count = 0;
    if (graph->data) memset(graph->data, 0,
                            (size_t)graph->max_points * sizeof(float));
}

static double graph_normalized(struct graph *graph, float value) {
    double range = graph->max_value - graph->min_value;
    if (range <= 0.0) return 0.0;
    double v = (value - graph->min_value) / range;
    return clamp(v, 0.0, 1.0);
}

void graph_draw(struct graph *graph, CGContextRef ctx, CGRect frame) {
    if (!graph || !ctx || graph->count < 2) return;

    CGContextSaveGState(ctx);

    int n = graph->count;
    float step = frame.size.width / (float)(n - 1);
    float base = frame.origin.y + frame.size.height;

    CGMutablePathRef path = CGPathCreateMutable();
    CGPathMoveToPoint(path, NULL,
                      frame.origin.x,
                      base - graph_normalized(graph, graph->data[0]) * frame.size.height);
    for (int i = 1; i < n; i++) {
        CGPathAddLineToPoint(path, NULL,
                             frame.origin.x + i * step,
                             base - graph_normalized(graph, graph->data[i]) * frame.size.height);
    }

    if (graph->fill_color.a > 0) {
        CGPathAddLineToPoint(path, NULL,
                             frame.origin.x + (n - 1) * step, base);
        CGPathAddLineToPoint(path, NULL, frame.origin.x, base);
        CGPathCloseSubpath(path);
        CGContextSetRGBFillColor(ctx,
                                 graph->fill_color.r,
                                 graph->fill_color.g,
                                 graph->fill_color.b,
                                 graph->fill_color.a);
        CGContextAddPath(ctx, path);
        CGContextFillPath(ctx);
        CGPathRelease(path);
        path = CGPathCreateMutable();
        CGPathMoveToPoint(path, NULL,
                          frame.origin.x,
                          base - graph_normalized(graph, graph->data[0]) * frame.size.height);
        for (int i = 1; i < n; i++) {
            CGPathAddLineToPoint(path, NULL,
                                 frame.origin.x + i * step,
                                 base - graph_normalized(graph, graph->data[i]) * frame.size.height);
        }
    }

    CGContextSetLineWidth(ctx, graph->line_width);
    CGContextSetLineJoin(ctx, kCGLineJoinRound);
    CGContextSetLineCap(ctx, kCGLineCapRound);
    CGContextSetRGBStrokeColor(ctx,
                               graph->line_color.r,
                               graph->line_color.g,
                               graph->line_color.b,
                               graph->line_color.a);
    CGContextAddPath(ctx, path);
    CGContextStrokePath(ctx);
    CGPathRelease(path);

    CGContextRestoreGState(ctx);
}