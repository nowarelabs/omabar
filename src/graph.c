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

void graph_draw(struct graph *graph, struct CGContext *ctx, CGRect frame) {
    (void)graph; (void)ctx; (void)frame;
}