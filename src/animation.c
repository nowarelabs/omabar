#include "animation.h"
#include "misc/helpers.h"
#include <QuartzCore/QuartzCore.h>
#include <stdlib.h>

static double animation_get_weight(enum animation_function fn, double t) {
    switch (fn) {
        case ANIMATION_LINEAR:
            return t;
        case ANIMATION_EASE_IN:
            return t * t * t;
        case ANIMATION_EASE_OUT:
            {
                double p = 1.0 - t;
                return 1.0 - p * p * p;
            }
        case ANIMATION_EASE_IN_OUT:
            {
                double p = 2.0 * t * t;
                return t < 0.5 ? p : p + (4.0 * t * t * t) - 6.0 * t * t + 1.0;
            }
        case ANIMATION_SPRING:
            {
                double c1 = 1.70158;
                double c3 = c1 + 1.0;
                double p = t - 1.0;
                return 1.0 + c3 * p * p * p + c1 * p * p;
            }
    }
    return t;
}

float animation_interpolate(enum animation_function fn, float initial, float final, double t) {
    return initial + (final - initial) * (float)animation_get_weight(fn, t);
}

void animation_init(struct animator *animator) {
    memset(animator, 0, sizeof(*animator));
}

void animation_destroy(struct animator *animator) {
    if (animator->display_link) {
        CVDisplayLinkStop(animator->display_link);
        CVDisplayLinkRelease(animator->display_link);
        animator->display_link = NULL;
    }
    if (animator->animations) {
        for (int i = 0; i < animator->animation_count; i++)
            free(animator->animations[i]);
        free(animator->animations);
        animator->animations = NULL;
        animator->animation_count = 0;
    }
}

void animation_run(struct animator *animator, struct animation *animation) {
    if (!animator || !animation) return;
    buf_push(animator->animations, animation);
    animation->started_at = CACurrentMediaTime();
}

void animation_cancel(struct animator *animator, struct animation *animation) {
    if (!animator || !animation) return;
    for (int i = 0; i < animator->animation_count; i++) {
        if (animator->animations[i] == animation) {
            memmove(&animator->animations[i], &animator->animations[i + 1],
                    (animator->animation_count - i - 1) * sizeof(struct animation *));
            animator->animation_count--;
            return;
        }
    }
}