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
    int cell_size;
    CGRect bounds;
};

void popup_create(struct popup *popup, struct bar_item *parent);
void popup_destroy(struct popup *popup);
void popup_add_item(struct popup *popup, struct bar_item *item);
void popup_remove_item(struct popup *popup, struct bar_item *item);
void popup_open(struct popup *popup);
void popup_close(struct popup *popup);
bool popup_has_items(struct popup *popup);
bool popup_is_open(struct popup *popup);
void popup_calculate_bounds(struct popup *popup);
void popup_draw(struct popup *popup, CGContextRef ctx);

#endif