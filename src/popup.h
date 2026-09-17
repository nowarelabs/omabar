#ifndef OMABAR_POPUP_H
#define OMABAR_POPUP_H

#include "bar_item.h"
#include "background.h"
#include <stdbool.h>

struct popup {
    struct bar_item **items;
    int item_count;
    struct background background;
    struct background item_background;
    bool is_open;
    bool drawing;       /* true = popup rendering enabled */
    int cell_size;
    int y_offset;       /* vertical offset below bar */
    float anchor_x;     /* x position of popup origin (in screen coords) */
    float anchor_y;     /* y position of popup origin (below bar) */
    CGRect bounds;      /* cached popup frame for hit-testing */
    struct bar_item *host; /* parent item that owns this popup */
};

struct bar;
void popup_init(struct popup *popup, struct bar_item *host);
void popup_destroy(struct popup *popup);
void popup_add_item(struct popup *popup, struct bar_item *item);
void popup_remove_item(struct popup *popup, struct bar_item *item);
void popup_open(struct popup *popup);
void popup_close(struct popup *popup);
void popup_toggle(struct popup *popup);
bool popup_has_items(struct popup *popup);
bool popup_is_open(struct popup *popup);
float popup_get_height(struct popup *popup);
void popup_calculate_bounds(struct popup *popup, float host_x, float host_y);
void popup_draw(struct popup *popup, struct bar *bar, CGContextRef ctx);
bool popup_item_at(struct popup *popup, float x, float y,
                   struct bar_item **out_item);
void popup_close_all(struct popup *popup);

#endif
