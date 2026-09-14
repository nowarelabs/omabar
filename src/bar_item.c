#include "bar_item.h"
#include "bar.h"
#include "bar_manager.h"
#include "misc/helpers.h"
#include <stdlib.h>

extern struct bar_manager g_bar_manager;

void bar_item_init(struct bar_item *item) {
    memset(item, 0, sizeof(*item));
    item->type = BAR_ITEM;
    item->position = POSITION_LEFT;
    item->click_enabled = 1;
    item->scroll_enabled = 0;
    item->padding_left = 2;
    item->padding_right = 2;
    item->update_mask = 0;
    text_init(&item->icon);
    text_init(&item->label);
    background_init(&item->background);
}

void bar_item_destroy(struct bar_item *item) {
    if (!item) return;
    free(item->name);
    text_destroy(&item->icon);
    text_destroy(&item->label);
    background_destroy(&item->background);
    if (item->graph) { graph_destroy(item->graph); free(item->graph); }
    if (item->alias) { alias_destroy(item->alias); free(item->alias); }
    if (item->slider) { slider_destroy(item->slider); free(item->slider); }
    if (item->popup) { popup_destroy(item->popup); free(item->popup); }
    free(item->script);
    free(item->click_script);
    free(item->windows);
}

void bar_item_draw(struct bar_item *item, struct bar *bar, struct CGContext *ctx) {
    (void)item; (void)bar; (void)ctx;
}

void bar_item_calculate_bounds(struct bar_item *item) {
    (void)item;
}

void bar_item_update(struct bar_item *item, const char *sender, const char *info) {
    (void)item; (void)sender; (void)info;
}