#include "popup.h"
#include "misc/helpers.h"
#include <stdlib.h>

void popup_create(struct popup *popup, struct bar_item *parent) {
    (void)parent;
    memset(popup, 0, sizeof(*popup));
    background_init(&popup->background);
    background_init(&popup->item_background);
}

void popup_destroy(struct popup *popup) {
    if (!popup) return;
    for (int i = 0; i < popup->item_count; i++)
        bar_item_destroy(popup->items[i]);
    free(popup->items);
    background_destroy(&popup->background);
    background_destroy(&popup->item_background);
}

void popup_add_item(struct popup *popup, struct bar_item *item) {
    if (!popup || !item) return;
    item->position = POSITION_POPUP;
    buf_push(popup->items, item);
}

void popup_open(struct popup *popup) {
    (void)popup;
}

void popup_close(struct popup *popup) {
    (void)popup;
}

bool popup_has_items(struct popup *popup) {
    return popup && popup->item_count > 0;
}