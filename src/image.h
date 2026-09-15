#ifndef OMABAR_IMAGE_H
#define OMABAR_IMAGE_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>

#include "color.h"

struct image {
    CGImageRef image;
    int border_width;
    int corner_radius;
    struct color border_color;
    char *path;
};

struct image *image_create(const char *path);
bool image_set_path(struct image *image, const char *path);
void image_destroy(struct image *image);
void image_draw(struct image *image, CGContextRef ctx, CGRect frame);

#endif