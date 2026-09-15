#ifndef OMABAR_ALIAS_H
#define OMABAR_ALIAS_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>

#include "color.h"

#define MENUBAR_LAYER 24

struct alias {
    int width;
    int height;
    int corner_radius;
    int disable_shadows;
    char *target_pid;
    char *owner;
    char *name;
    struct color background_color;
    bool inverse;
    CGRect window;
    CGImageRef image;
    int update_frequency;
    int counter;
    bool permission;
    bool is_forcing;
};

struct alias *alias_create(void);
void alias_destroy(struct alias *alias);
void alias_set_target(struct alias *alias, const char *owner, const char *name);
bool alias_request_permission(struct alias *alias);
bool alias_update(struct alias *alias, bool forced);
bool alias_update_image(struct alias *alias, bool forced);
void alias_draw(struct alias *alias, CGContextRef ctx, CGRect frame);

#endif