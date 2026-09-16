#include "bar.h"
#include "bar_manager.h"
#include "display.h"
#include "misc/extern.h"
#include "misc/helpers.h"
#include <stdlib.h>

extern struct bar_manager g_bar_manager;

CGRect bar_get_frame(struct bar *bar) {
    bool is_builtin = CGDisplayIsBuiltin(bar->did);
    int notch_offset = is_builtin ? g_bar_manager.notch_offset : 0;

    CGRect bounds = display_bounds(bar->did);
    CGPoint origin = bounds.origin;

    if (g_bar_manager.position == 2 /* left */
        || g_bar_manager.position == 3 /* right */) {
        bounds.size.height -= 2 * g_bar_manager.y_offset;

        origin.x += (g_bar_manager.position == 3
                     ? (bounds.size.width
                        - g_bar_manager.height
                        - g_bar_manager.margin)
                     : g_bar_manager.margin);
        origin.y += g_bar_manager.y_offset;

        return CGRectMake(origin.x, origin.y,
                          g_bar_manager.height,
                          bounds.size.height);
    }

    bounds.size.width -= 2 * g_bar_manager.margin;
    origin.x += g_bar_manager.margin;
    origin.y += g_bar_manager.y_offset + notch_offset;

    if (g_bar_manager.position == 1 /* bottom */) {
        origin.y = CGRectGetMaxY(bounds)
                   - g_bar_manager.height
                   - 2 * g_bar_manager.y_offset
                   - notch_offset;
    }

    return CGRectMake(origin.x, origin.y, bounds.size.width, g_bar_manager.height);
}

static void bar_create_window(struct bar *bar) {
    CGRect frame = bar_get_frame(bar);
    bar->window = window_open(bar,
                              (int)frame.size.width,
                              (int)frame.size.height);
    if (!bar->window) return;

    window_set_frame(bar->window,
                     (int)frame.origin.x,
                     (int)frame.origin.y,
                     (int)frame.size.width,
                     (int)frame.size.height);
    window_assign_mouse_tracking_area(bar->window, frame);
    window_set_blur_radius(bar->window, g_bar_manager.blur_radius);
    if (!g_bar_manager.shadow)
        window_disable_shadow(bar->window);
    if (g_bar_manager.topmost)
        window_set_level(bar->window, 101); /* kCGMainMenuWindowLevel */
}

struct bar *bar_create(unsigned int did) {
    /* ensure SLS connection is available before display queries */
    extern uint32_t g_connection;
    if (g_connection == 0)
        g_connection = SLSMainConnectionID();

    struct bar *bar = calloc(1, sizeof(*bar));
    if (!bar) return NULL;

    bar->did = did;
    bar->adid = display_arrangement_id(did);
    bar->dsid = display_space_id(did);
    bar->shown = 1;
    bar->hidden = 0;
    bar->mouse_over = 0;
    bar->background = g_bar_manager.default_item.background;
    bar->x_offset = 0;

    bar_create_window(bar);
    return bar;
}

void bar_destroy(struct bar *bar) {
    if (!bar) return;
    if (bar->window) {
        window_close(bar->window);
        window_destroy(bar->window);
    }
    free(bar);
}

/* Display hotplug: create/remove bars without freeing managed memory.
   Creates a bar for `did` if none exists, appending to the manager.
   Returns 1 if a bar was created, 0 otherwise. */
int bar_manager_refresh_display(struct bar_manager *bm, unsigned int did) {
    for (int i = 0; i < bm->bar_count; i++) {
        if (bm->bars[i]->did == did) return 0;
    }
    struct bar *bar = bar_create(did);
    if (!bar) return 0;
    buf_push(bm->bars, bar);
    bm->bar_count = (int)buf_len(bm->bars);
    return 1;
}

void bar_destroy_for_display(struct bar_manager *bm, unsigned int did) {
    for (int i = 0; i < bm->bar_count; i++) {
        if (bm->bars[i]->did != did) continue;
        bar_destroy(bm->bars[i]);
        buf_del(bm->bars, i);
        bm->bar_count = (int)buf_len(bm->bars);
        return;
    }
}

void bar_calculate_bounds(struct bar *bar) {
    if (!bar) return;
    if (bar->adid < 1) return;

    bool is_builtin = CGDisplayIsBuiltin(bar->did);
    uint32_t notch_width = is_builtin ? (uint32_t)g_bar_manager.notch_width : 0;

    int bar_w = (int)bar->window->frame.size.width;
    int bar_h = (int)bar->window->frame.size.height;

    /* up-front measure of the slots we place items into */
    int left_first     = 0;
    int right_first    = bar_w;
    int center_first   = bar_w / 2;
    int center_r_first = (bar_w + (int)notch_width) / 2;
    int center_l_first = (bar_w - (int)notch_width) / 2;

    struct bar_manager *bm = &g_bar_manager;

    /* measure total center length first (left/right grow independently) */
    int center_total = 0;
    for (int i = 0; i < bm->bar_item_count; i++) {
        struct bar_item *item = bm->bar_items[i];
        if (!item || item->hidden) continue;
        bar_item_calculate_bounds(item);
        int len = (int)bar_item_get_length(item);
        if (item->position == POSITION_CENTER)
            center_total += len + item->background.padding_right;
    }
    center_first   = (bar_w - center_total) / 2;
    center_r_first = (bar_w + (int)notch_width) / 2;
    center_l_first = (bar_w - (int)notch_width) / 2;

    for (int i = 0; i < bm->bar_item_count; i++) {
        struct bar_item *item = bm->bar_items[i];
        if (!item || item->hidden) continue;
        if (item->position == POSITION_POPUP) continue;

        bar_item_calculate_bounds(item);
        int len = (int)bar_item_get_length(item);

        int *cursor = NULL;
        int rtl = 0;

        switch (item->position) {
            case POSITION_LEFT:        cursor = &left_first; break;
            case POSITION_CENTER:      cursor = &center_first; break;
            case POSITION_RIGHT:       cursor = &right_first; rtl = 1; break;
            case POSITION_CENTER_RIGHT:cursor = &center_r_first; break;
            case POSITION_CENTER_LEFT: cursor = &center_l_first; rtl = 1; break;
            default: continue;
        }

        if (rtl) {
            *cursor = *cursor - len - item->background.padding_right;
            item->x = *cursor;
        } else {
            item->x = *cursor + item->padding_left;
            *cursor += len + item->background.padding_right;
        }
        (void)bar_h;
    }

    bar->x_offset = bar->window->origin.x;
}

void bar_draw(struct bar *bar) {
    if (!bar || !bar->window || !bar->window->context) return;
    if (bar->adid < 1 || bar->hidden) return;

    CGContextRef ctx = bar->window->context;
    CGRect frame = bar->window->frame;

    /* clear the window back to transparent */
    CGContextSaveGState(ctx);
    CGContextClearRect(ctx, frame);
    CGContextRestoreGState(ctx);

    /* bar-wide background */
    if (bar->background.type > 0 || bar->background.color.a > 0
        || bar->background.has_shadow) {
        CGRect bg_frame = CGRectMake(0, 0, frame.size.width, frame.size.height);
        background_draw(&bar->background, ctx, bg_frame);
    }

    /* draw each visible item at its computed position */
    struct bar_manager *bm = &g_bar_manager;
    for (int i = 0; i < bm->bar_item_count; i++) {
        struct bar_item *item = bm->bar_items[i];
        if (!item || item->hidden) continue;

        /* save, translate to item's slot, draw, restore */
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, item->x, 0);
        bar_item_draw(item, bar, ctx);
        CGContextRestoreGState(ctx);

        if (item->popup && item->popup->is_open)
            popup_draw(item->popup, ctx);
    }

    CGContextFlush(ctx);
}