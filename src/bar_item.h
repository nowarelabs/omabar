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
struct window;

/* update_mask bits (match custom_events builtin registration order) */
#define UPDATE_ROUTINE          (1ULL << 0)
#define UPDATE_FORCED           (1ULL << 1)
#define UPDATE_MOUSE_ENTERED    (1ULL << 2)
#define UPDATE_MOUSE_EXITED     (1ULL << 3)
#define UPDATE_MOUSE_SCROLLED   (1ULL << 4)
#define UPDATE_MOUSE_CLICKED    (1ULL << 5)
#define UPDATE_VOLUME_CHANGED   (1ULL << 6)
#define UPDATE_POWER_CHANGED    (1ULL << 7)
#define UPDATE_WIFI_CHANGED     (1ULL << 8)
#define UPDATE_BRIGHTNESS_CHANGED (1ULL << 9)
#define UPDATE_MEDIA_CHANGED    (1ULL << 10)
#define UPDATE_FRONT_APP_SWITCHED (1ULL << 11)
#define UPDATE_SPACE_CHANGED    (1ULL << 12)
#define UPDATE_DISPLAY_ADDED    (1ULL << 13)
#define UPDATE_DISPLAY_REMOVED  (1ULL << 14)
#define UPDATE_DISPLAY_MOVED    (1ULL << 15)
#define UPDATE_WINDOW_FOCUSED   (1ULL << 16)
#define UPDATE_SCROLL_TICK      (1ULL << 17)
#define UPDATE_ANIMATION_TICK   (1ULL << 18)
#define UPDATE_DAEMON_MESSAGE   (1ULL << 19)
#define UPDATE_DND              (1ULL << 20)

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

    /* space component state */
    uint64_t space_id;
    int selected;
    char **icon_strip;
    int icon_strip_count;

    char *script;
    char *click_script;
    void *mach_helper;

    int hidden;
    int mouse_over;
    int click_enabled;
    int scroll_enabled;
    int y_offset;
    int padding_left;
    int padding_right;
    int label_x_offset;
    int icon_x_offset;
    int update_interval;
    int index;
    double x;
    uint64_t tick_counter;   /* frames since last "routine" update */

    /* scroll animation state */
    float scroll_offset;     /* visual displacement driven by scroll animation */
    float scroll_sensitivity; /* pixels per scroll delta unit; 0 = disabled */
    float *scroll_values;    /* array of target values to cycle through on scroll */
    int scroll_value_count;  /* number of entries in scroll_values */
    int scroll_index;        /* current position in scroll_values */
};

void bar_item_init(struct bar_item *item);
void bar_item_destroy(struct bar_item *item);
struct bar_item *bar_item_clone(const struct bar_item *src);
struct window *bar_item_get_window(struct bar_item *item, int adid);
float bar_item_get_content_length(struct bar_item *item);
float bar_item_get_length(struct bar_item *item);
void bar_item_draw(struct bar_item *item, struct bar *bar, CGContextRef ctx);
CGRect bar_item_calculate_bounds(struct bar_item *item);
void bar_item_update(struct bar_item *item, const char *sender, const char *info);
void bar_item_on_click(struct bar_item *item, uint32_t button, uint32_t modifier, CGPoint point);
void bar_item_on_scroll(struct bar_item *item, int scroll_delta, uint32_t modifier);
void bar_item_mouse_entered(struct bar_item *item);
void bar_item_mouse_exited(struct bar_item *item);

/* property setters (each marks the item for refresh) */
void bar_item_set_name(struct bar_item *item, const char *name);
void bar_item_set_icon(struct bar_item *item, const char *string);
void bar_item_set_label(struct bar_item *item, const char *string);
void bar_item_set_icon_font(struct bar_item *item, struct font *font);
void bar_item_set_label_font(struct bar_item *item, struct font *font);
void bar_item_set_icon_color(struct bar_item *item, struct color color);
void bar_item_set_label_color(struct bar_item *item, struct color color);
void bar_item_set_background_color(struct bar_item *item, struct color color);
void bar_item_set_background_border_color(struct bar_item *item, struct color color);
void bar_item_set_background_corner_radius(struct bar_item *item, int radius);
void bar_item_set_background_border_width(struct bar_item *item, int width);
void bar_item_set_background_height(struct bar_item *item, int height);
void bar_item_set_shadow(struct bar_item *item, int enabled);
void bar_item_set_position(struct bar_item *item, enum bar_item_position position);
void bar_item_set_update_mask(struct bar_item *item, uint64_t mask);
void bar_item_set_script(struct bar_item *item, const char *script);
void bar_item_set_click_script(struct bar_item *item, const char *script);
void bar_item_set_hidden(struct bar_item *item, int hidden);
void bar_item_set_click_enabled(struct bar_item *item, int enabled);
void bar_item_set_scroll_enabled(struct bar_item *item, int enabled);
void bar_item_set_y_offset(struct bar_item *item, int offset);
void bar_item_set_padding(struct bar_item *item, int left, int right, int top, int bottom);
void bar_item_set_label_x_offset(struct bar_item *item, int offset);
void bar_item_set_icon_x_offset(struct bar_item *item, int offset);
void bar_item_set_index(struct bar_item *item, int index);

/* event subscription — set/unset individual bits in update_mask;
   subscribing to a system event also ensures the source is active */
void bar_item_event_subscribe(struct bar_item *item, const char *event_name);
void bar_item_event_unsubscribe(struct bar_item *item, const char *event_name);

/* space component */
void bar_item_set_type(struct bar_item *item, enum bar_item_type type);
void bar_item_set_icon_strip(struct bar_item *item, char **strip, int count);
void bar_item_set_space_id(struct bar_item *item, uint64_t sid);
void bar_item_set_selected(struct bar_item *item, int selected);
int  bar_item_space_draw(struct bar_item *item, struct bar *bar, CGContextRef ctx, CGRect frame);
void bar_item_space_clicked(struct bar_item *item);

#endif