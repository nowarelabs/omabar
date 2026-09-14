#ifndef OMABAR_IMAGE_H
#define OMABAR_IMAGE_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>

struct image {
    CGImageRef image;
    int border_width;
    int corner_radius;
    char *path;
};

struct image *image_create(const char *path);
void image_destroy(struct image *image);
void image_draw(struct image *image, struct CGContext *ctx, CGRect frame);

#endif