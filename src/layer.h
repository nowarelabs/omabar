#ifndef OMABAR_LAYER_H
#define OMABAR_LAYER_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>

struct layer {
    void *context;   /* CAContextRef */
    void *root;      /* CALayerRef  */
    uint32_t caid;   /* CAContext id */
};

struct layer *layer_create(uint32_t cid);
void layer_destroy(struct layer *layer);
void layer_set_contents(struct layer *layer, CGImageRef image);
void layer_set_alpha(struct layer *layer, float alpha);

#endif