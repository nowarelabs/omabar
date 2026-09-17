#ifndef OMABAR_BAR_MANAGER_H
#define OMABAR_BAR_MANAGER_H

#include "bar.h"
#include "bar_item.h"
#include "event_loop.h"
#include "custom_events.h"
#include "animation.h"

struct bar_manager {
    struct bar **bars;
    int bar_count;

    struct bar_item **bar_items;
    int bar_item_count;

    struct bar_item default_item;

    struct custom_events custom_events;
    struct animator animator;

    /* bar-wide settings */
    int position;       /* 0=top, 1=bottom, 2=left, 3=right */
    int height;
    int width;
    int margin;
    int blur_radius;
    int shadow;
    int topmost;
    int sticky;
    int notch_width;
    int notch_offset;
    int y_offset;
    double alpha;

    /* state */
    int frozen;
    int sleeps;
    int bar_needs_update;
    int bar_needs_resize;
    int needs_ordering;
};

void bar_manager_init(struct bar_manager *bm);
void bar_manager_begin(struct bar_manager *bm);
void bar_manager_destroy(struct bar_manager *bm);
void bar_manager_register_event_handlers(void);
void bar_manager_refresh(struct bar_manager *bm);
void bar_manager_set_needs_update(struct bar_manager *bm);
void bar_manager_set_needs_resize(struct bar_manager *bm);

/* item management */
int  bar_manager_add_item(struct bar_manager *bm, struct bar_item *item);
void bar_manager_remove_item(struct bar_manager *bm, const char *name);
void bar_manager_move_item(struct bar_manager *bm, const char *name, const char *ref, const char *position);
void bar_manager_reorder_item(struct bar_manager *bm, const char *name, int new_index);
void bar_manager_clone_item(struct bar_manager *bm, const char *name);
void bar_manager_rename_item(struct bar_manager *bm, const char *old_name, const char *new_name);

/* property setters */
void bar_manager_set_position(struct bar_manager *bm, int position);
void bar_manager_set_height(struct bar_manager *bm, int height);
void bar_manager_set_width(struct bar_manager *bm, int width);
void bar_manager_set_margin(struct bar_manager *bm, int margin);
void bar_manager_set_blur_radius(struct bar_manager *bm, int blur);
void bar_manager_set_shadow(struct bar_manager *bm, int shadow);
void bar_manager_set_topmost(struct bar_manager *bm, int topmost);
void bar_manager_set_sticky(struct bar_manager *bm, int sticky);
void bar_manager_set_notch_width(struct bar_manager *bm, int width);
void bar_manager_set_notch_offset(struct bar_manager *bm, int offset);
void bar_manager_set_y_offset(struct bar_manager *bm, int offset);
void bar_manager_set_alpha(struct bar_manager *bm, double alpha);
void bar_manager_set_background_color(struct bar_manager *bm, struct color color);

#endif
