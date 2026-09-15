#include "popup.h"
#include "misc/helpers.h"
#include <stdlib.h>

void popup_create(struct popup *popup, struct bar_item *parent) {
    (void)parent;
    memset(popup, 0, sizeof(*popup));
    background_init(&popup->background);
    background_init(&popup->item_background);
    popup->cell_size = 30;
    popup->is_open = false;
}

void popup_destroy(struct popup *popup) {
    if (!popup) return;
    buf_free(popup->items);
    popup->items = NULL;
    popup->item_count = 0;
    background_destroy(&popup->background);
    background_destroy(&popup->item_background);
}

void popup_add_item(struct popup *popup, struct bar_item *item) {
    if (!popup || !item) return;
    item->position = POSITION_POPUP;
    buf_push(popup->items, item);
    popup->item_count = buf_len(popup->items);
}

void popup_remove_item(struct popup *popup, struct bar_item *item) {
    if (!popup || !item || !popup->items) return;
    for (int i = 0; i < popup->item_count; i++) {
        if (popup->items[i] == item) {
            buf_del(popup->items, i);
            popup->item_count = buf_len(popup->items);
            return;
        }
    }
}

void popup_open(struct popup *popup) {
    if (!popup) return;
    popup->is_open = true;
}

void popup_close(struct popup *popup) {
    if (!popup) return;
    popup->is_open = false;
}

bool popup_has_items(struct popup *popup) {
    return popup && popup->item_count > 0;
}

bool popup_is_open(struct popup *popup) {
    return popup && popup->is_open;
}

void popup_calculate_bounds(struct popup *popup) {
    if (!popup) return;
    int visible = 0;
    for (int i = 0; i < popup->item_count; i++) {
        if (!popup->items[i]->hidden) visible++;
    }
    float pad_t = (float)popup->background.padding_top
                + (float)popup->background.border_width;
    float pad_b = (float)popup->background.padding_bottom
                + (float)popup->background.border_width;
    float pad_l = (float)popup->background.padding_left
                + (float)popup->background.border_width;
    float pad_r = (float)popup->background.padding_right
                + (float)popup->background.border_width;
    float max_w = popup->bounds.size.width;
    float total_h = pad_t + pad_b;

    for (int i = 0; i < popup->item_count; i++) {
        struct bar_item *item = popup->items[i];
        if (item->hidden) continue;
        total_h += (float)popup->cell_size;
    }

    popup->bounds = (CGRect){{ 0, 0 },
                             { max_w > 0 ? max_w : pad_l + pad_r,
                               total_h }};
}

void popup_draw(struct popup *popup, CGContextRef ctx) {
    if (!popup || !ctx || !popup->is_open) return;
    popup_calculate_bounds(popup);
    background_draw(&popup->background, ctx, popup->bounds);
}