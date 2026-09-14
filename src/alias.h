#ifndef OMABAR_ALIAS_H
#define OMABAR_ALIAS_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>

#include "color.h"

struct alias {
    int width;
    int height;
    int corner_radius;
    int disable_shadows;
    char *target_pid;
    int is_forcing;
    struct color background_color;
    bool inverse;
    CGRect window;
};

struct alias *alias_create(void);
void alias_destroy(struct alias *alias);
void alias_draw(struct alias *alias, struct CGContext *ctx, CGRect frame);

#endif