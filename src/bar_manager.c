#include "bar_manager.h"
#include "bar.h"
#include "main.h"
#include "display.h"
#include "event.h"
#include "workspace.h"
#include "dnd.h"
#include "misc/helpers.h"
#include <CoreGraphics/CoreGraphics.h>
#include <stdio.h>
#include <stdlib.h>

/* misc/defines.h defines EVENT_MOUSE_* as config string literals; undef so the
   event.h enum values win here for handler registration. */
#undef EVENT_MOUSE_ENTERED
#undef EVENT_MOUSE_EXITED
#undef EVENT_MOUSE_CLICKED
#undef EVENT_MOUSE_SCROLLED

#define MAX_DISPLAYS 8

static void bar_manager_handle_display(const struct event *event) {
    struct bar_manager *bm = &g_bar_manager;
    if (!event) return;

    if (event->type == EVENT_DISPLAY_ADDED) {
        CGDirectDisplayID did = (CGDirectDisplayID)event->arg1;
        if (bar_manager_refresh_display(bm, did))
            bm->bar_needs_update = 1;
    } else if (event->type == EVENT_DISPLAY_REMOVED) {
        CGDirectDisplayID did = (CGDirectDisplayID)event->arg1;
        bar_destroy_for_display(bm, did);
        bm->bar_needs_update = 1;
    } else if (event->type == EVENT_DISPLAY_MOVED) {
        CGDirectDisplayID did = (CGDirectDisplayID)event->arg1;
        for (int i = 0; i < bm->bar_count; i++) {
            struct bar *bar = bm->bars[i];
            if (bar && bar->did == did) {
                CGRect frame = bar_get_frame(bar);
                if (bar->window)
                    window_set_frame(bar->window, frame.origin.x, frame.origin.y,
                                     frame.size.width, frame.size.height);
                break;
            }
        }
        bm->bar_needs_update = 1;
    }
}

/* iterate items matching a mask, run their script updates */
static void bar_manager_update_items(uint64_t mask, const char *sender) {
    struct bar_manager *bm = &g_bar_manager;
    for (int i = 0; i < bm->bar_item_count; i++) {
        struct bar_item *item = bm->bar_items[i];
        if (item && (item->update_mask & mask))
            bar_item_update(item, sender, NULL);
    }
}

static void bar_manager_handle_space_changed(const struct event *event) {
    (void)event;
    struct bar_manager *bm = &g_bar_manager;
    uint64_t sid = event ? event->arg1 : 0;

    for (int i = 0; i < bm->bar_count; i++) {
        struct bar *bar = bm->bars[i];
        if (!bar || !bar->did) continue;
        uint64_t new_sid = sid ? sid : display_space_id(bar->did);
        if (new_sid) bar->sid = new_sid;
        bar->dsid = display_space_display_id(bar->sid);
    }
    bar_manager_update_items(UPDATE_SPACE_CHANGED, "space_changed");
    bm->bar_needs_update = 1;
}

static void bar_manager_handle_window_focused(const struct event *event) {
    (void)event;
    bar_manager_update_items(UPDATE_WINDOW_FOCUSED, "window_focused");
    bar_manager_update_items(UPDATE_FRONT_APP_SWITCHED, "front_app_switched");
    bar_manager_set_needs_update(&g_bar_manager);
}

static void bar_manager_handle_volume_changed(const struct event *event) {
    (void)event;
    bar_manager_update_items(UPDATE_VOLUME_CHANGED, "volume_changed");
    bar_manager_set_needs_update(&g_bar_manager);
}

static void bar_manager_handle_power_changed(const struct event *event) {
    (void)event;
    bar_manager_update_items(UPDATE_POWER_CHANGED, "power_changed");
    bar_manager_set_needs_update(&g_bar_manager);
}

static void bar_manager_handle_wifi_changed(const struct event *event) {
    (void)event;
    bar_manager_update_items(UPDATE_WIFI_CHANGED, "wifi_changed");
    bar_manager_set_needs_update(&g_bar_manager);
}

static void bar_manager_handle_media_changed(const struct event *event) {
    (void)event;
    bar_manager_update_items(UPDATE_MEDIA_CHANGED, "media_changed");
    bar_manager_set_needs_update(&g_bar_manager);
}

static void bar_manager_handle_brightness_changed(const struct event *event) {
    (void)event;
    bar_manager_update_items(UPDATE_BRIGHTNESS_CHANGED, "brightness_changed");
    bar_manager_set_needs_update(&g_bar_manager);
}

/*
 * Items subscribed to a custom event (e.g. "dnd") re-run their script
 * with SENDER set to the event name. dispatch by resolving name -> mask.
 */
static void bar_manager_handle_custom_event(const struct event *event) {
    struct bar_manager *bm = &g_bar_manager;
    if (!event || !event->data) return;

    uint64_t mask = custom_events_get_mask(&bm->custom_events,
                                           (const char *)event->data);
    if (!mask) return;

    for (int i = 0; i < bm->bar_item_count; i++) {
        struct bar_item *item = bm->bar_items[i];
        if (item && (item->update_mask & mask))
            bar_item_update(item, (const char *)event->data, NULL);
    }
    bar_manager_set_needs_update(bm);
}

/*
 * Per-frame tick from the CVDisplayLink (animation_begin). Items with a
 * nonzero update_interval refresh every `update_interval` frames; on a
 * 60 Hz display 60 => once per second (clock, battery, ...).
 */
static unsigned int g_tick_dnd_counter = 0;

static void bar_manager_handle_scroll_tick(const struct event *event) {
    (void)event;
    struct bar_manager *bm = &g_bar_manager;

    /* re-read DND state once per second (60 ticks) */
    if (++g_tick_dnd_counter >= 60) {
        g_tick_dnd_counter = 0;
        dnd_update();
    }

    for (int i = 0; i < bm->bar_item_count; i++) {
        struct bar_item *item = bm->bar_items[i];
        if (!item || item->update_interval <= 0) continue;

        item->tick_counter++;
        if (item->tick_counter < (uint64_t)item->update_interval) continue;

        item->tick_counter = 0;
        bar_item_update(item, NULL, NULL);
    }

    if (bm->bar_item_count > 0)
        bar_manager_refresh(bm);
}

static struct bar *bar_manager_bar_at(struct bar_manager *bm, CGPoint point) {
    for (int i = 0; i < bm->bar_count; i++) {
        struct bar *bar = bm->bars[i];
        if (!bar || !bar->window) continue;
        CGRect frame = bar->window->frame;
        frame.origin = bar->window->origin;
        if (CGRectContainsPoint(frame, point))
            return bar;
    }
    return NULL;
}

static struct bar_item *bar_manager_item_at(struct bar *bar, CGPoint point) {
    struct bar_manager *bm = &g_bar_manager;
    if (!bar) return NULL;
    double local_x = point.x - bar->window->origin.x;
    for (int i = 0; i < bm->bar_item_count; i++) {
        struct bar_item *item = bm->bar_items[i];
        if (!item || item->hidden) continue;
        if (local_x >= item->x
            && local_x <= item->x + bar_item_get_length(item))
            return item;
    }
    return NULL;
}

static void bar_manager_handle_mouse_entered(const struct event *event) {
    struct bar_manager *bm = &g_bar_manager;
    if (!event || !event->data) return;
    CGPoint *point = (CGPoint *)event->data;
    struct bar *bar = bar_manager_bar_at(bm, *point);
    struct bar_item *item = bar_manager_item_at(bar, *point);
    if (item && !item->mouse_over) {
        item->mouse_over = 1;
        bar_item_mouse_entered(item);
    }
}

static void bar_manager_handle_mouse_exited(const struct event *event) {
    struct bar_manager *bm = &g_bar_manager;
    if (!event || !event->data) return;
    CGPoint *point = (CGPoint *)event->data;
    struct bar *bar = bar_manager_bar_at(bm, *point);
    struct bar_item *item = bar_manager_item_at(bar, *point);
    (void)item;
    for (int i = 0; i < bm->bar_item_count; i++) {
        struct bar_item *it = bm->bar_items[i];
        if (it && it->mouse_over) {
            it->mouse_over = 0;
            bar_item_mouse_exited(it);
        }
    }
}

static void bar_manager_handle_mouse_scrolled(const struct event *event) {
    (void)event;
    bar_manager_update_items(UPDATE_MOUSE_SCROLLED, "mouse.scrolled");
    bar_manager_set_needs_update(&g_bar_manager);
}

static void bar_manager_handle_mouse_clicked(const struct event *event) {
    struct bar_manager *bm = &g_bar_manager;
    if (!event || !event->data) return;
    CGPoint *point = (CGPoint *)event->data;
    struct bar *bar = bar_manager_bar_at(bm, *point);
    struct bar_item *item = bar_manager_item_at(bar, *point);
    if (!item) return;
    bar_item_on_click(item, (uint32_t)event->arg1, (uint32_t)event->arg2, *point);
    bar_manager_set_needs_update(bm);
}

void bar_manager_init(struct bar_manager *bm) {
    memset(bm, 0, sizeof(*bm));

    /* defaults */
    bm->position = 0;      /* top */
    bm->height = 40;
    bm->width = 0;
    bm->margin = 0;
    bm->blur_radius = 0;
    bm->shadow = 0;
    bm->topmost = 1;
    bm->sticky = 1;
    bm->notch_width = 0;
    bm->notch_offset = 0;
    bm->y_offset = 0;
    bm->alpha = 1.0;

    bm->frozen = 0;
    bm->sleeps = 0;
    bm->bar_needs_update = 0;
    bm->bar_needs_resize = 0;
    bm->needs_ordering = 0;

    /* default item template */
    bar_item_init(&bm->default_item);

    event_init();
    custom_events_init(&bm->custom_events);
    animation_init(&bm->animator);
}

void bar_manager_begin(struct bar_manager *bm) {
    if (!bm) return;
    if (bm->sleeps) return;

    event_handler_fn fn = bar_manager_handle_display;
    g_event_handler[EVENT_DISPLAY_ADDED] = fn;
    g_event_handler[EVENT_DISPLAY_REMOVED] = fn;
    g_event_handler[EVENT_DISPLAY_MOVED] = fn;
    g_event_handler[EVENT_SPACE_CHANGED] = bar_manager_handle_space_changed;
    g_event_handler[EVENT_WINDOW_FOCUSED] = bar_manager_handle_window_focused;
    g_event_handler[EVENT_APP_FRONT_SWITCHED] = bar_manager_handle_window_focused;
    g_event_handler[EVENT_VOLUME_CHANGED] = bar_manager_handle_volume_changed;
    g_event_handler[EVENT_POWER_CHANGED] = bar_manager_handle_power_changed;
    g_event_handler[EVENT_WIFI_CHANGED] = bar_manager_handle_wifi_changed;
    g_event_handler[EVENT_MEDIA_CHANGED] = bar_manager_handle_media_changed;
    g_event_handler[EVENT_BRIGHTNESS_CHANGED] = bar_manager_handle_brightness_changed;
    g_event_handler[EVENT_SCROLL_TICK] = bar_manager_handle_scroll_tick;
    g_event_handler[EVENT_CUSTOM] = bar_manager_handle_custom_event;
    g_event_handler[EVENT_MOUSE_ENTERED] = bar_manager_handle_mouse_entered;
    g_event_handler[EVENT_MOUSE_EXITED] = bar_manager_handle_mouse_exited;
    g_event_handler[EVENT_MOUSE_SCROLLED] = bar_manager_handle_mouse_scrolled;
    g_event_handler[EVENT_MOUSE_CLICKED] = bar_manager_handle_mouse_clicked;

    display_begin();
    workspace_event_handler_begin();
    animation_begin(&bm->animator);
    dnd_init();

    /* create a bar for every active display */
    uint32_t count = 0;
    CGDirectDisplayID displays[MAX_DISPLAYS];
    if (CGGetActiveDisplayList((uint32_t)MAX_DISPLAYS, displays, &count)
            == kCGErrorSuccess) {
        for (uint32_t i = 0; i < count; i++)
            if (displays[i])
                bar_manager_refresh_display(bm, displays[i]);
    }

    bm->bar_needs_update = 1;
}

void bar_manager_destroy(struct bar_manager *bm) {
    for (int i = 0; i < bm->bar_count; i++)
        bar_destroy(bm->bars[i]);
    buf_free(bm->bars);
    bm->bars = NULL;
    bm->bar_count = 0;

    for (int i = 0; i < bm->bar_item_count; i++)
        bar_item_destroy(bm->bar_items[i]);
    buf_free(bm->bar_items);
    bm->bar_items = NULL;
    bm->bar_item_count = 0;

    custom_events_destroy(&bm->custom_events);
    animation_destroy(&bm->animator);
}

void bar_manager_refresh(struct bar_manager *bm) {
    if (!bm || bm->sleeps) return;

    if (bm->frozen) {
        bm->bar_needs_update = 1;
        return;
    }

    /* resize pass: recreate windows if size changed */
    if (bm->bar_needs_resize) {
        for (int i = 0; i < bm->bar_count; i++) {
            struct bar *bar = bm->bars[i];
            if (!bar || !bar->window) continue;
            CGRect frame = bar_get_frame(bar);
            window_set_frame(bar->window, frame.origin.x, frame.origin.y,
                             frame.size.width, frame.size.height);
        }
        bm->bar_needs_resize = 0;
        bm->bar_needs_update = 1;
    }

    /* bounds + draw pass */
    if (bm->bar_needs_update) {
        for (int i = 0; i < bm->bar_count; i++) {
            struct bar *bar = bm->bars[i];
            if (!bar || bar->hidden) continue;
            bar_calculate_bounds(bar);
            bar_draw(bar);
        }
        bm->bar_needs_update = 0;
    }

    /* ordering pass */
    if (bm->needs_ordering) {
        for (int i = 0; i < bm->bar_count; i++) {
            struct bar *bar = bm->bars[i];
            if (!bar || !bar->window) continue;
            int level = bm->topmost ? 101 : 0;
            window_set_level(bar->window, level);
        }
        bm->needs_ordering = 0;
        bm->bar_needs_update = 1;
    }
}

void bar_manager_set_needs_update(struct bar_manager *bm) {
    if (bm) bm->bar_needs_update = 1;
}

void bar_manager_set_needs_resize(struct bar_manager *bm) {
    if (bm) bm->bar_needs_resize = 1;
}

static struct bar_item *bar_manager_find_item(struct bar_manager *bm, const char *name) {
    if (!bm || !name) return NULL;
    for (int i = 0; i < bm->bar_item_count; i++)
        if (bm->bar_items[i]->name && string_equals(bm->bar_items[i]->name, name))
            return bm->bar_items[i];
    return NULL;
}

int bar_manager_add_item(struct bar_manager *bm, struct bar_item *item) {
    if (!bm || !item || !item->name) return 1;
    if (bar_manager_find_item(bm, item->name)) return 1;
    buf_push(bm->bar_items, item);
    bm->bar_item_count = (int)buf_len(bm->bar_items);
    bm->bar_needs_update = 1;
    return 0;
}

void bar_manager_remove_item(struct bar_manager *bm, const char *name) {
    if (!bm || !name) return;
    for (int i = 0; i < bm->bar_item_count; i++) {
        if (!bm->bar_items[i]->name || !string_equals(bm->bar_items[i]->name, name))
            continue;
        bar_item_destroy(bm->bar_items[i]);
        buf_del(bm->bar_items, i);
        bm->bar_item_count = (int)buf_len(bm->bar_items);
        bm->bar_needs_update = 1;
        return;
    }
}

void bar_manager_move_item(struct bar_manager *bm, const char *name, const char *ref, const char *position) {
    if (!bm || !name || !ref || !position) return;

    struct bar_item *item = bar_manager_find_item(bm, name);
    struct bar_item *ref_item = bar_manager_find_item(bm, ref);
    if (!item || !ref_item || item == ref_item) return;

    int item_idx = -1;
    for (int i = 0; i < bm->bar_item_count; i++)
        if (bm->bar_items[i] == item) { item_idx = i; break; }
    if (item_idx < 0) return;

    /* remove item from array without freeing it */
    struct bar_item *saved = bm->bar_items[item_idx];
    buf_del(bm->bar_items, item_idx);

    /* find ref position */
    int ref_idx = -1;
    for (int i = 0; i < bm->bar_item_count; i++)
        if (bm->bar_items[i] == ref_item) { ref_idx = i; break; }

    int insert_at;
    if (string_equals(position, "front"))
        insert_at = ref_idx;
    else if (string_equals(position, "behind"))
        insert_at = ref_idx + 1;
    else
        insert_at = ref_idx + 1;

    buf_push(bm->bar_items, saved);
    /* shift: pop from end, insert at insert_at */
    struct bar_item *last = bm->bar_items[bm->bar_item_count - 1];
    for (int i = bm->bar_item_count - 1; i > insert_at; i--)
        bm->bar_items[i] = bm->bar_items[i - 1];
    bm->bar_items[insert_at] = last;
    bm->bar_needs_update = 1;
}

void bar_manager_reorder_item(struct bar_manager *bm, const char *name, int new_index) {
    if (!bm || !name || new_index < 0) return;

    int item_idx = -1;
    struct bar_item *item = NULL;
    for (int i = 0; i < bm->bar_item_count; i++)
        if (bm->bar_items[i]->name && string_equals(bm->bar_items[i]->name, name)) {
            item = bm->bar_items[i];
            item_idx = i;
            break;
        }
    if (!item) return;

    buf_del(bm->bar_items, item_idx);
    if (new_index > bm->bar_item_count) new_index = bm->bar_item_count;
    buf_push(bm->bar_items, item);
    struct bar_item *last = bm->bar_items[bm->bar_item_count - 1];
    for (int i = bm->bar_item_count - 1; i > new_index; i--)
        bm->bar_items[i] = bm->bar_items[i - 1];
    bm->bar_items[new_index] = last;
    bm->bar_needs_update = 1;
}

void bar_manager_clone_item(struct bar_manager *bm, const char *name) {
    struct bar_item *src = bar_manager_find_item(bm, name);
    if (!src) return;

    struct bar_item *dup = bar_item_clone(src);
    /* generate a unique name */
    char buf[256];
    snprintf(buf, sizeof(buf), "%s_%d", src->name, bm->bar_item_count);
    bar_item_set_name(dup, buf);

    buf_push(bm->bar_items, dup);
    bm->bar_item_count = (int)buf_len(bm->bar_items);
    bm->bar_needs_update = 1;
}

void bar_manager_rename_item(struct bar_manager *bm, const char *old_name, const char *new_name) {
    struct bar_item *item = bar_manager_find_item(bm, old_name);
    if (!item) return;
    bar_item_set_name(item, new_name);
    bm->bar_needs_update = 1;
}

void bar_manager_set_position(struct bar_manager *bm, int position) {
    if (bm->position == position) return;
    bm->position = position;
    bm->bar_needs_resize = 1;
}

void bar_manager_set_height(struct bar_manager *bm, int height) {
    if (bm->height == height) return;
    bm->height = height;
    bm->bar_needs_resize = 1;
}

void bar_manager_set_width(struct bar_manager *bm, int width) {
    if (bm->width == width) return;
    bm->width = width;
    bm->bar_needs_resize = 1;
}

void bar_manager_set_margin(struct bar_manager *bm, int margin) {
    if (bm->margin == margin) return;
    bm->margin = margin;
    bm->bar_needs_update = 1;
}

void bar_manager_set_blur_radius(struct bar_manager *bm, int blur) {
    if (bm->blur_radius == blur) return;
    bm->blur_radius = blur;
    bm->bar_needs_update = 1;
}

void bar_manager_set_shadow(struct bar_manager *bm, int shadow) {
    if (bm->shadow == shadow) return;
    bm->shadow = shadow;
    bm->bar_needs_update = 1;
}

void bar_manager_set_topmost(struct bar_manager *bm, int topmost) {
    if (bm->topmost == topmost) return;
    bm->topmost = topmost;
    bm->needs_ordering = 1;
}

void bar_manager_set_sticky(struct bar_manager *bm, int sticky) {
    if (bm->sticky == sticky) return;
    bm->sticky = sticky;
    bm->needs_ordering = 1;
}

void bar_manager_set_notch_width(struct bar_manager *bm, int width) {
    if (bm->notch_width == width) return;
    bm->notch_width = width;
    bm->bar_needs_resize = 1;
}

void bar_manager_set_notch_offset(struct bar_manager *bm, int offset) {
    if (bm->notch_offset == offset) return;
    bm->notch_offset = offset;
    bm->bar_needs_resize = 1;
}

void bar_manager_set_y_offset(struct bar_manager *bm, int offset) {
    if (bm->y_offset == offset) return;
    bm->y_offset = offset;
    bm->bar_needs_update = 1;
}

void bar_manager_set_alpha(struct bar_manager *bm, double alpha) {
    if (bm->alpha == alpha) return;
    bm->alpha = alpha;
    bm->bar_needs_update = 1;
}

void bar_manager_set_background_color(struct bar_manager *bm, struct color color) {
    if (!bm) return;
    struct background *bg = &bm->default_item.background;
    if (color_is_valid(color)) {
        bg->color = color;
    } else {
        background_init(bg);
    }
    bm->bar_needs_update = 1;
}