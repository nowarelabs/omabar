#ifndef OMABAR_ANIMATION_H
#define OMABAR_ANIMATION_H

#include <CoreVideo/CoreVideo.h>
#include <stdbool.h>

#include "color.h"

enum animation_function {
    ANIMATION_LINEAR,
    ANIMATION_EASE_IN,
    ANIMATION_EASE_OUT,
    ANIMATION_EASE_IN_OUT,
    ANIMATION_SPRING
};

enum animation_type {
    ANIMATION_FLOAT,
    ANIMATION_COLOR,
    ANIMATION_ALPHA
};

struct animation {
    void *target;
    enum animation_type type;
    enum animation_function function;
    float initial;
    float final;
    float current;
    double duration;
    double started_at;
    struct color color_initial;
    struct color color_final;
    struct animation *next; /* chained animation run on completion */
};

struct animator {
    CVDisplayLinkRef display_link;
    struct animation **animations;
    int animation_count;
};

void animation_init(struct animator *animator);
void animation_begin(struct animator *animator);
void animation_destroy(struct animator *animator);
void animation_run(struct animator *animator, struct animation *animation);
void animation_cancel(struct animator *animator, struct animation *animation);
int  animation_tick(struct animator *animator);
float animation_interpolate(enum animation_function fn, float initial, float final, double t);

#endif