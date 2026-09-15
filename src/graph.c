#include "graph.h"
#include "misc/helpers.h"
#include <stdlib.h>

struct graph *graph_create(int width, int height) {
    struct graph *g = calloc(1, sizeof(*g));
    if (!g) return NULL;
    g->width = width;
    g->height = height;
    g->fill_color = color_from_hex(0x00000000);
    g->line_color = color_from_hex(0xffffffff);
    g->data = calloc(GRAPH_MAX_POINTS, sizeof(float));
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

void graph_push_value(struct graph *graph, float value) {
    if (!graph) return;
    if (graph->count >= GRAPH_MAX_POINTS) {
        memmove(graph->data, graph->data + 1,
                (GRAPH_MAX_POINTS - 1) * sizeof(float));
        graph->count = GRAPH_MAX_POINTS - 1;
    }
    graph->data[graph->count++] = value;
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
                      base - graph->data[0] * frame.size.height);
    for (int i = 1; i < n; i++) {
        CGPathAddLineToPoint(path, NULL,
                             frame.origin.x + i * step,
                             base - graph->data[i] * frame.size.height);
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
                          base - graph->data[0] * frame.size.height);
        for (int i = 1; i < n; i++) {
            CGPathAddLineToPoint(path, NULL,
                                 frame.origin.x + i * step,
                                 base - graph->data[i] * frame.size.height);
        }
    }

    CGContextSetLineWidth(ctx, 1.0f);
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