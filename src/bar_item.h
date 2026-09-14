#ifndef OMABAR_BAR_ITEM_H
#define OMABAR_BAR_ITEM_H

#include "text.h"
#include "background.h"
#include "graph.h"
#include "alias.h"
#include "slider.h"
#include "group.h"
#include "popup.h"

struct bar;

enum bar_item_type {
    BAR_ITEM,
    BAR_COMPONENT_SPACE,
    BAR_COMPONENT_ALIAS,
    BAR_COMPONENT_GROUP,
    BAR_COMPONENT_GRAPH,
    BAR_COMPONENT_SLIDER
};

enum bar_item_position {
    POSITION_LEFT,
    POSITION_CENTER,
    POSITION_RIGHT,
    POSITION_CENTER_LEFT,
    POSITION_CENTER_RIGHT,
    POSITION_POPUP
};

struct bar_item {
    enum bar_item_type type;
    char *name;

    struct text icon;
    struct text label;
    struct background background;

    struct graph *graph;
    struct alias *alias;
    struct slider *slider;
    struct group *group;
    struct popup *popup;

    struct window **windows;
    int window_count;

    enum bar_item_position position;
    uint32_t associated_space;
    uint32_t associated_display;
    uint32_t associated_bar;
    uint64_t update_mask;

    char *script;
    char *click_script;
    void *mach_helper;

    int hidden;
    int click_enabled;
    int scroll_enabled;
    int y_offset;
    int padding_left;
    int padding_right;
    int label_x_offset;
    int icon_x_offset;
    int update_interval;
    int index;
};

void bar_item_init(struct bar_item *item);
void bar_item_destroy(struct bar_item *item);
void bar_item_draw(struct bar_item *item, struct bar *bar, struct CGContext *ctx);
void bar_item_calculate_bounds(struct bar_item *item);
void bar_item_update(struct bar_item *item, const char *sender, const char *info);

#endif