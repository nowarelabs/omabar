#include "bar_manager.h"
#include "event.h"
#include "misc/helpers.h"
#include <stdlib.h>

void bar_manager_init(struct bar_manager *bm) {
    memset(bm, 0, sizeof(*bm));

    /* defaults */
    bm->position = 0;      /* top */
    bm->height = 40;
    bm->width = 0;
    bm->margin = 0;
    bm->blur_radius = 0;
    bm->shadow = 0;
    bm->topmost = 1;
    bm->sticky = 1;
    bm->notch_width = 0;
    bm->notch_offset = 0;
    bm->y_offset = 0;
    bm->alpha = 1.0;

    bm->frozen = 0;
    bm->sleeps = 0;
    bm->bar_needs_update = 0;
    bm->bar_needs_resize = 0;
    bm->needs_ordering = 0;

    event_init();
    custom_events_init(&bm->custom_events);
    animation_init(&bm->animator);
}

void bar_manager_begin(struct bar_manager *bm) {
    (void)bm;
}

void bar_manager_destroy(struct bar_manager *bm) {
    for (int i = 0; i < bm->bar_count; i++)
        bar_destroy(bm->bars[i]);
    free(bm->bars);
    bm->bars = NULL;
    bm->bar_count = 0;

    for (int i = 0; i < bm->bar_item_count; i++)
        bar_item_destroy(bm->bar_items[i]);
    free(bm->bar_items);
    bm->bar_items = NULL;
    bm->bar_item_count = 0;

    custom_events_destroy(&bm->custom_events);
    animation_destroy(&bm->animator);
}

void bar_manager_refresh(struct bar_manager *bm) {
    (void)bm;
}

int bar_manager_add_item(struct bar_manager *bm, struct bar_item *item) {
    buf_push(bm->bar_items, item);
    return 0;
}

void bar_manager_remove_item(struct bar_manager *bm, const char *name) {
    (void)bm;
    (void)name;
}

void bar_manager_move_item(struct bar_manager *bm, const char *name, const char *ref, const char *position) {
    (void)bm; (void)name; (void)ref; (void)position;
}

void bar_manager_set_position(struct bar_manager *bm, int position) {
    if (bm->position == position) return;
    bm->position = position;
    bm->bar_needs_resize = 1;
}

void bar_manager_set_height(struct bar_manager *bm, int height) {
    if (bm->height == height) return;
    bm->height = height;
    bm->bar_needs_resize = 1;
}

void bar_manager_set_width(struct bar_manager *bm, int width) {
    if (bm->width == width) return;
    bm->width = width;
    bm->bar_needs_resize = 1;
}

void bar_manager_set_margin(struct bar_manager *bm, int margin) {
    if (bm->margin == margin) return;
    bm->margin = margin;
    bm->bar_needs_update = 1;
}

void bar_manager_set_blur_radius(struct bar_manager *bm, int blur) {
    if (bm->blur_radius == blur) return;
    bm->blur_radius = blur;
    bm->bar_needs_update = 1;
}

void bar_manager_set_shadow(struct bar_manager *bm, int shadow) {
    if (bm->shadow == shadow) return;
    bm->shadow = shadow;
    bm->bar_needs_update = 1;
}

void bar_manager_set_topmost(struct bar_manager *bm, int topmost) {
    if (bm->topmost == topmost) return;
    bm->topmost = topmost;
    bm->needs_ordering = 1;
}

void bar_manager_set_sticky(struct bar_manager *bm, int sticky) {
    if (bm->sticky == sticky) return;
    bm->sticky = sticky;
    bm->needs_ordering = 1;
}

void bar_manager_set_notch_width(struct bar_manager *bm, int width) {
    if (bm->notch_width == width) return;
    bm->notch_width = width;
    bm->bar_needs_resize = 1;
}

void bar_manager_set_notch_offset(struct bar_manager *bm, int offset) {
    if (bm->notch_offset == offset) return;
    bm->notch_offset = offset;
    bm->bar_needs_resize = 1;
}

void bar_manager_set_y_offset(struct bar_manager *bm, int offset) {
    if (bm->y_offset == offset) return;
    bm->y_offset = offset;
    bm->bar_needs_update = 1;
}

void bar_manager_set_alpha(struct bar_manager *bm, double alpha) {
    if (bm->alpha == alpha) return;
    bm->alpha = alpha;
    bm->bar_needs_update = 1;
}