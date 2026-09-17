#include "popup.h"
#include "bar.h"
#include "bar_manager.h"
#include "misc/helpers.h"
#include <stdlib.h>
#include <string.h>

void popup_init(struct popup *popup, struct bar_item *host) {
    if (!popup) return;
    memset(popup, 0, sizeof(*popup));
    popup->host = host;
    popup->cell_size = 30;
    popup->is_open = false;
    background_init(&popup->background);
    background_init(&popup->item_background);
}

void popup_destroy(struct popup *popup) {
    if (!popup) return;
    buf_free(popup->items);
    popup->items = NULL;
    popup->item_count = 0;
    popup->host = NULL;
    background_destroy(&popup->background);
    background_destroy(&popup->item_background);
}

static bool popup_has_item(struct popup *popup, struct bar_item *item) {
    for (int i = 0; i < popup->item_count; i++) {
        if (popup->items[i] == item) return true;
    }
    return false;
}

void popup_add_item(struct popup *popup, struct bar_item *item) {
    if (!popup || !item) return;
    if (popup_has_item(popup, item)) return;
    item->position = POSITION_POPUP;
    buf_push(popup->items, item);
    popup->item_count = buf_len(popup->items);
    popup->drawing = true;
}

void popup_remove_item(struct popup *popup, struct bar_item *item) {
    if (!popup || !item || !popup->items) return;
    for (int i = 0; i < popup->item_count; i++) {
        if (popup->items[i] == item) {
            buf_del(popup->items, i);
            popup->item_count = buf_len(popup->items);
            if (popup->item_count == 0)
                popup->drawing = false;
            return;
        }
    }
}

void popup_open(struct popup *popup) {
    if (!popup || popup->is_open) return;
    popup->is_open = true;
    popup->drawing = true;
}

void popup_close(struct popup *popup) {
    if (!popup) return;
    popup->is_open = false;
}

void popup_toggle(struct popup *popup) {
    if (!popup) return;
    if (popup->is_open) popup_close(popup);
    else popup_open(popup);
}

bool popup_has_items(struct popup *popup) {
    return popup && popup->item_count > 0;
}

bool popup_is_open(struct popup *popup) {
    return popup && popup->is_open;
}

float popup_get_height(struct popup *popup) {
    if (!popup) return 0.0f;
    int visible = 0;
    for (int i = 0; i < popup->item_count; i++) {
        if (!popup->items[i]->hidden) visible++;
    }
    float pad_t = (float)popup->background.padding_top
                + (float)popup->background.border_width;
    float pad_b = (float)popup->background.padding_bottom
                + (float)popup->background.border_width;
    return pad_t + pad_b + (float)visible * (float)popup->cell_size;
}

void popup_calculate_bounds(struct popup *popup, float host_x, float host_y) {
    if (!popup) return;

    float pad_l = (float)popup->background.padding_left
                + (float)popup->background.border_width;
    float pad_r = (float)popup->background.padding_right
                + (float)popup->background.border_width;

    float width = pad_l + pad_r;
    for (int i = 0; i < popup->item_count; i++) {
        struct bar_item *item = popup->items[i];
        if (item->hidden) continue;
        float cell_w = (float)item->padding_left + bar_item_get_content_length(item)
                       + (float)item->padding_right;
        if (cell_w > width) width = cell_w;
    }

    popup->anchor_x = host_x;
    popup->anchor_y = host_y;
    popup->bounds = CGRectMake(host_x, host_y, width, popup_get_height(popup));
}

static void popup_draw_children(struct popup *popup, struct bar *bar,
                                CGContextRef ctx) {
    float x = popup->bounds.origin.x
              + (float)popup->background.padding_left
              + (float)popup->background.border_width;
    float y = popup->bounds.origin.y
              + (float)popup->background.padding_top
              + (float)popup->background.border_width;

    for (int i = 0; i < popup->item_count; i++) {
        struct bar_item *item = popup->items[i];
        if (!item || item->hidden) continue;

        CGRect cell = CGRectMake(x, y, popup->bounds.size.width, popup->cell_size);

        CGContextSaveGState(ctx);
        if (popup->item_background.color.a > 0
            || popup->item_background.type > 0) {
            background_draw(&popup->item_background, ctx, cell);
        }
        CGContextTranslateCTM(ctx, x, y);
        bar_item_draw(item, bar, ctx);
        CGContextRestoreGState(ctx);

        if (item->popup && item->popup->is_open) {
            item->popup->anchor_x = x;
            item->popup->anchor_y = CGRectGetMaxY(cell);
            popup_calculate_bounds(item->popup, x,
                                   CGRectGetMaxY(cell));
            popup_draw(item->popup, bar, ctx);
        }

        y += (float)popup->cell_size;
    }
}

void popup_draw(struct popup *popup, struct bar *bar, CGContextRef ctx) {
    if (!popup || !ctx || !popup->is_open || popup->item_count == 0) return;
    if (!popup->host) return;
    popup_calculate_bounds(popup, popup->anchor_x, popup->anchor_y);

    /* popup background (no shadow inside the shared bar window) */
    bool shadow = popup->background.has_shadow;
    popup->background.has_shadow = false;
    background_draw(&popup->background, ctx, popup->bounds);
    popup->background.has_shadow = shadow;

    popup_draw_children(popup, bar, ctx);
}

bool popup_item_at(struct popup *popup, float x, float y,
                   struct bar_item **out_item) {
    if (!popup || !popup->is_open || popup->item_count == 0) return false;
    if (!CGRectContainsPoint(popup->bounds, CGPointMake(x, y))) return false;

    float cy = popup->bounds.origin.y
               + (float)popup->background.padding_top
               + (float)popup->background.border_width;
    for (int i = 0; i < popup->item_count; i++) {
        struct bar_item *item = popup->items[i];
        if (!item || item->hidden) continue;
        if (y >= cy && y <= cy + (float)popup->cell_size) {
            /* nested popup takes priority over the parent cell */
            if (item->popup && item->popup->is_open) {
                struct bar_item *deep = NULL;
                if (popup_item_at(item->popup, x, y, &deep) && deep) {
                    if (out_item) *out_item = deep;
                    return true;
                }
            }
            if (out_item) *out_item = item;
            return true;
        }
        cy += (float)popup->cell_size;
    }
    return false;
}

void popup_close_all(struct popup *popup) {
    if (!popup) return;
    for (int i = 0; i < popup->item_count; i++) {
        if (popup->items[i] && popup->items[i]->popup)
            popup_close_all(popup->items[i]->popup);
    }
    popup_close(popup);
}