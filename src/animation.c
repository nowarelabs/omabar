#include "animation.h"
#include "event.h"
#include "misc/helpers.h"
#include <QuartzCore/QuartzCore.h>
#include <stdlib.h>

/*
 * Per-frame tick source: one CVDisplayLink posts a SCROLL_TICK event
 * every display refresh (~60 fps on the active CG displays). Items with
 * a nonzero update_interval accumulate a counter and refresh every
 * `update_interval` frames (e.g. 60 => once per second for a clock).
 */
static CVReturn animation_frame_callback(CVDisplayLinkRef display_link,
                                         const CVTimeStamp *now,
                                         const CVTimeStamp *output_time,
                                         CVOptionFlags flags,
                                         CVOptionFlags *flags_out,
                                         void *context) {
    (void)display_link; (void)now; (void)flags; (void)flags_out;
    uint64_t host_time = output_time->hostTime;

    dispatch_async(dispatch_get_main_queue(), ^{
        struct event event = {
            .type = EVENT_SCROLL_TICK,
            .arg1 = host_time
        };
        event_post(&event);
        (void)context;
    });
    return kCVReturnSuccess;
}

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

void animation_begin(struct animator *animator) {
    if (animator->display_link) return;

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    CVDisplayLinkRef link = NULL;
    if (CVDisplayLinkCreateWithActiveCGDisplays(&link) != kCVReturnSuccess)
        return;

    CVDisplayLinkSetOutputCallback(link, animation_frame_callback, animator);
    CVDisplayLinkStart(link);
    #pragma clang diagnostic pop
    animator->display_link = link;
}

static void animation_free_chain(struct animation *a) {
    while (a) {
        struct animation *next = a->next;
        free(a);
        a = next;
    }
}

void animation_destroy(struct animator *animator) {
    if (animator->display_link) {
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        CVDisplayLinkStop(animator->display_link);
        CVDisplayLinkRelease(animator->display_link);
        #pragma clang diagnostic pop
        animator->display_link = NULL;
    }
    if (animator->animations) {
        for (size_t i = 0; i < buf_len(animator->animations); i++)
            animation_free_chain(animator->animations[i]);
        buf_free(animator->animations);
        animator->animations = NULL;
        animator->animation_count = 0;
    }
}

void animation_run(struct animator *animator, struct animation *animation) {
    if (!animator || !animation) return;
    buf_push(animator->animations, animation);
    animator->animation_count = buf_len(animator->animations);
    animation->started_at = CACurrentMediaTime();
}

void animation_cancel(struct animator *animator, struct animation *animation) {
    if (!animator || !animation) return;
    for (int i = 0; i < animator->animation_count; i++) {
        if (animator->animations[i] == animation) {
            animation_free_chain(animator->animations[i]);
            buf_del(animator->animations, i);
            animator->animation_count = buf_len(animator->animations);
            return;
        }
    }
}

int animation_tick(struct animator *animator) {
    if (!animator || animator->animation_count == 0) return 0;

    double now = CACurrentMediaTime();
    int remaining = 0;

    for (int i = 0; i < animator->animation_count; i++) {
        struct animation *a = animator->animations[i];
        if (!a) continue;

        double elapsed = now - a->started_at;
        double t = a->duration > 0.0 ? elapsed / a->duration : 1.0;
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;

        float value = animation_interpolate(a->function, a->initial, a->final, t);
        if (a->target) *(float *)a->target = value;

        if (t >= 1.0) {
            if (a->next) {
                /* run the chained animation next: inherit the ending state */
                a->next->initial = a->final;
                a->next->started_at = now;
                animator->animations[i] = a->next;
                a->next = NULL;
                free(a);
                remaining = 1;
                continue;
            }
            free(a);
            buf_del(animator->animations, i);
            i--;
            animator->animation_count = buf_len(animator->animations);
        } else {
            remaining = 1;
        }
    }
    return remaining;
}