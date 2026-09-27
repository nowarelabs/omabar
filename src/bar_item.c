#include "bar_item.h"
#include "bar.h"
#include "bar_manager.h"
#include "event.h"
#include "mach.h"
#include "misc/helpers.h"
#include "misc/extern.h"
#include "window.h"
#include "volume.h"
#include "power.h"
#include "wifi.h"
#include "media.h"
#include "display.h"
#include "app_windows.h"
#include "app_menus.h"
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

extern struct bar_manager g_bar_manager;

/* ---- built-in vector icons --------------------------------------------
   Drawn with CoreGraphics rather than glyphs so the default theme needs no
   icon font. Each returns the width it occupies; the caller centres it in
   the content row. */

/* ---- built-in vector icons --------------------------------------------
   Drawn with CoreGraphics rather than glyphs so the default theme needs no
   icon font. Each draws into `box` and returns the width it occupies; the
   caller positions it in the content row. */

static void icon_set_stroke(struct bar_item *item, CGContextRef ctx) {
    CGContextSetRGBFillColor(ctx, item->icon.color.r, item->icon.color.g,
                             item->icon.color.b, item->icon.color.a);
    CGContextSetRGBStrokeColor(ctx, item->icon.color.r, item->icon.color.g,
                               item->icon.color.b, item->icon.color.a);
    CGContextSetLineWidth(ctx, 1.2f);
    CGContextSetLineCap(ctx, kCGLineCapRound);
}

static CGFloat icon_clamp_radius(CGRect rect, CGFloat radius) {
    if (radius > rect.size.height / 2.0f || radius > rect.size.width / 2.0f)
        radius = rect.size.height < rect.size.width
                 ? rect.size.height / 2.0f : rect.size.width / 2.0f;
    return radius;
}

static void icon_fill_rounded(CGContextRef ctx, CGRect rect, CGFloat radius) {
    CGFloat r = icon_clamp_radius(rect, radius);
    CGPathRef path = CGPathCreateWithRoundedRect(rect, r, r, NULL);
    CGContextAddPath(ctx, path);
    CGContextFillPath(ctx);
    CGPathRelease(path);
}

static void icon_stroke_rounded(CGContextRef ctx, CGRect rect, CGFloat radius) {
    CGFloat r = icon_clamp_radius(rect, radius);
    CGPathRef path = CGPathCreateWithRoundedRect(rect, r, r, NULL);
    CGContextAddPath(ctx, path);
    CGContextStrokePath(ctx);
    CGPathRelease(path);
}

static float bar_item_icon_battery(struct bar_item *item, CGContextRef ctx,
                                   CGRect box) {
    /* body + terminal nub + a fill proportional to the charge */
    int charge = power_get_charge();
    if (charge < 0) charge = 0;
    if (charge > 100) charge = 100;

    CGFloat w = box.size.width;
    CGFloat h = box.size.height;
    CGFloat nub = 1.5f;
    CGRect body = CGRectMake(box.origin.x, box.origin.y, w - nub - 1.0f, h);

    icon_set_stroke(item, ctx);
    icon_stroke_rounded(ctx, body, 2.5f);

    CGFloat nub_h = h / 3.0f;
    CGContextFillRect(ctx, CGRectMake(CGRectGetMaxX(body),
                                      box.origin.y + nub_h, nub,
                                      h - 2.0f * nub_h));

    CGFloat pad = 1.8f;
    CGFloat fill_w = (body.size.width - 2.0f * pad)
                    * ((CGFloat)charge / 100.0f);
    if (fill_w > 0.0f) {
        CGRect fill = CGRectMake(body.origin.x + pad, body.origin.y + pad,
                                 fill_w, body.size.height - 2.0f * pad);
        icon_fill_rounded(ctx, fill, 1.0f);
    }
    return w;
}

static float bar_item_icon_volume(struct bar_item *item, CGContextRef ctx,
                                  CGRect box) {
    /* speaker cone plus one or two arcs for the level */
    int pct = volume_get_percentage();
    if (pct < 0) pct = 0;

    CGFloat h = box.size.height;
    CGFloat y = box.origin.y;
    CGFloat x = box.origin.x;
    CGFloat horn = h * 0.40f;
    CGFloat body_w = horn * 0.45f;

    icon_set_stroke(item, ctx);

    /* speaker: small rect body, then a triangular horn */
    CGContextFillRect(ctx, CGRectMake(x, y + h * 0.32f, body_w, h * 0.36f));
    CGContextMoveToPoint(ctx, x + body_w, y + h * 0.50f);
    CGContextAddLineToPoint(ctx, x + body_w + horn, y + h * 0.08f);
    CGContextAddLineToPoint(ctx, x + body_w + horn, y + h * 0.92f);
    CGContextClosePath(ctx);
    CGContextFillPath(ctx);

    /* waves to the right of the horn: none when muted, up to two otherwise */
    CGFloat cx = x + body_w + horn + 1.0f;
    CGFloat cy = y + h * 0.5f;
    int arcs = (pct > 66) ? 2 : (pct > 33 ? 1 : 0);
    for (int i = 0; i < arcs; i++) {
        CGFloat r = h * (0.26f + 0.22f * (i + 1));
        CGContextAddArc(ctx, cx, cy, r,
                        (CGFloat)(-M_PI / 3.0f), (CGFloat)(M_PI / 3.0f), 0);
        CGContextStrokePath(ctx);
    }
    return h * 1.2f;
}

static float bar_item_icon_wifi(struct bar_item *item, CGContextRef ctx,
                                CGRect box) {
    CGFloat h = box.size.height;
    CGFloat cx = box.origin.x + h / 2.0f;
    CGFloat cy = box.origin.y + h;
    CGFloat r_max = h * 0.95f;

    icon_set_stroke(item, ctx);
    /* arcs open upward: in this y-down space that is 225deg -> 315deg */
    for (int i = 0; i < 3; i++) {
        CGFloat r = r_max * (0.34f + 0.33f * i);
        CGContextAddArc(ctx, cx, cy, r,
                        (CGFloat)(5.0 * M_PI / 4.0),
                        (CGFloat)(7.0 * M_PI / 4.0), 0);
        CGContextStrokePath(ctx);
    }
    CGFloat dot = 1.3f;
    CGContextFillEllipseInRect(ctx, CGRectMake(cx - dot, cy - dot,
                                              dot * 2.0f, dot * 2.0f));
    return h;
}

static float bar_item_icon_media(struct bar_item *item, CGContextRef ctx,
                                 CGRect box) {
    /* eighth note: two filled stems joined by a beam, plus note heads */
    CGFloat h = box.size.height;
    CGFloat x = box.origin.x;
    CGFloat y = box.origin.y;

    icon_set_stroke(item, ctx);
    CGContextSetLineWidth(ctx, 1.1f);

    CGFloat head = h * 0.30f;
    CGFloat left_x = x + h * 0.12f;
    CGFloat right_x = x + h * 0.66f;
    CGFloat top = y + h * 0.06f;

    /* stems */
    CGContextMoveToPoint(ctx, left_x, y + h * 0.66f);
    CGContextAddLineToPoint(ctx, left_x, top);
    CGContextMoveToPoint(ctx, right_x, y + h * 0.48f);
    CGContextAddLineToPoint(ctx, right_x, top);
    CGContextStrokePath(ctx);

    /* beam */
    CGContextFillRect(ctx, CGRectMake(left_x, top, right_x - left_x, 1.1f));

    /* note heads */
    CGContextFillEllipseInRect(ctx, CGRectMake(left_x - head * 0.7f,
                                              y + h * 0.62f, head, head));
    CGContextFillEllipseInRect(ctx, CGRectMake(right_x - head * 0.7f,
                                              y + h * 0.44f, head, head));
    return h * 0.95f;
}

/* Which vector icon, if any, this item should draw. An explicit glyph from
   the config always wins, so icon_strip / icon.string still take priority. */
static enum bar_item_kind bar_item_vector_icon(struct bar_item *item) {
    if (item->kind == BAR_KIND_DIVIDER) return BAR_KIND_GENERIC;
    if (item->icon.string && item->icon.string[0]) return BAR_KIND_GENERIC;
    if (item->icon_strip && item->icon_strip_count > 0) return BAR_KIND_GENERIC;
    if (item->kind == BAR_KIND_BATTERY
        || item->kind == BAR_KIND_VOLUME
        || item->kind == BAR_KIND_WIFI
        || item->kind == BAR_KIND_MEDIA)
        return item->kind;
    return BAR_KIND_GENERIC;
}

/* Width the item's vector icon will occupy, or 0 when it uses text glyphs. */
float bar_item_icon_width(struct bar_item *item) {
    if (item->kind == BAR_KIND_DIVIDER)
        return (float)(item->divider_width > 0 ? item->divider_width : 1);
    if (bar_item_vector_icon(item) == BAR_KIND_GENERIC) return 0.0f;
    CGFloat size = (CGFloat)BAR_ITEM_ROW_HEIGHT * 0.52f;
    if (item->kind == BAR_KIND_VOLUME) return (float)(size * 1.2f);
    if (item->kind == BAR_KIND_MEDIA)  return (float)(size * 0.95f);
    return (float)size;
}

/* Draws the item's vector icon (if any) at the start of `row`, returning the
   width consumed so the label flow accounts for it. */
static float bar_item_draw_vector_icon(struct bar_item *item, CGContextRef ctx,
                                       CGRect row) {
    enum bar_item_kind kind = bar_item_vector_icon(item);
    if (kind == BAR_KIND_GENERIC) return 0.0f;

    CGFloat size = row.size.height * 0.52f;
    CGRect box = CGRectMake(row.origin.x + (CGFloat)item->icon_x_offset,
                            row.origin.y + (row.size.height - size) / 2.0f,
                            size, size);

    switch (kind) {
    case BAR_KIND_BATTERY: return bar_item_icon_battery(item, ctx, box);
    case BAR_KIND_VOLUME:  return bar_item_icon_volume(item, ctx, box);
    case BAR_KIND_WIFI:    return bar_item_icon_wifi(item, ctx, box);
    case BAR_KIND_MEDIA:   return bar_item_icon_media(item, ctx, box);
    default: return 0.0f;
    }
}

/* A divider is just a centred hairline: no text, no icon. Drawn at the
   item's own left padding, then the draw pass ends. */
static void bar_item_draw_divider(struct bar_item *item, CGContextRef ctx,
                                  CGRect row) {
    CGFloat w = item->divider_width > 0 ? (CGFloat)item->divider_width : 1.0f;
    CGFloat inset = row.size.height * 0.22f;
    struct color c = color_is_valid(item->divider_color)
                     ? item->divider_color : item->icon.color;
    CGContextSetRGBFillColor(ctx, c.r, c.g, c.b, c.a);
    CGContextFillRect(ctx, CGRectMake((CGFloat)item->padding_left,
                                      row.origin.y + inset, w,
                                      row.size.height - 2.0f * inset));
}

/* Fill in an item's own content for the built-in kinds. A configured script
   always wins, so this only runs when the item has none: it is what makes
   clock/battery/volume/wifi/media/front_app show values out of the box
   instead of rendering empty. */
void bar_item_apply_builtin_content(struct bar_item *item) {
    if (!item || item->script) return;

    char buf[128];

    switch (item->kind) {
    case BAR_KIND_CLOCK: {
        time_t now = time(NULL);
        struct tm tm_buf;
        struct tm *tm = localtime_r(&now, &tm_buf);
        if (!tm) return;
        const char *fmt = item->format && item->format[0]
                          ? item->format : "%d/%m %H:%M";
        if (strftime(buf, sizeof(buf), fmt, tm) == 0) return;
        text_set_string(&item->label, buf);
        break;
    }

    case BAR_KIND_BATTERY: {
        int charge = power_get_charge();
        if (charge < 0) return;
        snprintf(buf, sizeof(buf), "%d%%", charge);
        text_set_string(&item->label, buf);
        break;
    }

    case BAR_KIND_VOLUME: {
        int pct = volume_get_percentage();
        if (pct < 0) return;
        snprintf(buf, sizeof(buf), "%d%%", pct);
        text_set_string(&item->label, buf);
        break;
    }

    case BAR_KIND_WIFI: {
        const char *ssid = wifi_get_ssid();
        text_set_string(&item->label, (ssid && ssid[0]) ? ssid : "");
        break;
    }

    case BAR_KIND_FRONT_APP: {
        const char *app = app_windows_front_app_name();
        text_set_string(&item->label, (app && app[0]) ? app : "");
        break;
    }

    case BAR_KIND_MEDIA: {
        struct media_info *info = media_get_info();
        if (info) {
            /* "Artist - Title" reads best in a bar; fall back to whichever
               field is populated. */
            if (info->artist && info->artist[0] && info->title && info->title[0])
                snprintf(buf, sizeof(buf), "%s - %s", info->artist, info->title);
            else if (info->title && info->title[0])
                snprintf(buf, sizeof(buf), "%s", info->title);
            else if (info->artist && info->artist[0])
                snprintf(buf, sizeof(buf), "%s", info->artist);
            else if (info->app && info->app[0])
                snprintf(buf, sizeof(buf), "%s", info->app);
            else
                buf[0] = '\0';
            media_free_info(info);
        } else {
            buf[0] = '\0';
        }
        text_set_string(&item->label, buf);
        /* Nothing playing: take the whole item out of the layout rather
           than leaving a bare note icon behind. */
        item->hidden = (buf[0] == '\0');
        break;
    }

    case BAR_KIND_APP_LOGO: {
        /* The system Apple menu is always menu 0, so its presence is the only
           thing to check. */
        int menus = app_menus_refresh();
        item->hidden = !(menus > OMABAR_APP_MENU_APPLE);
        break;
    }

    case BAR_KIND_APP_MENUS: {
        /* A different app offers a different set of menus, so this item's
           width changes with the front app; hidden entirely when the app
           offers none beyond its own name. */
        int menus = app_menus_refresh();
        int count = menus - item->app_menu_first;
        item->hidden = !(count > 0);
        break;
    }

    case BAR_KIND_APP_MENU_ITEM:
        /* popup leaves are filled in when the menu is opened, not here */
        return;

    default:
        return;
    }
}

void bar_item_init(struct bar_item *item) {
    memset(item, 0, sizeof(*item));
    item->type = BAR_ITEM;
    item->position = POSITION_LEFT;
    item->click_enabled = 1;
    item->scroll_enabled = 0;
    item->scroll_sensitivity = 10.0f;
    item->padding_left = 2;
    item->padding_right = 2;
    item->update_mask = 0;
    item->space_count = 1;
    item->space_gap = 4;
    /* menus 0 and 1 are the Apple and app menus, drawn by the app_logo and
       front_app kinds; an app_menus item starts past both */
    item->app_menu_first = OMABAR_APP_MENU_FIRST;
    item->divider_width = 1;
    text_init(&item->icon);
    text_init(&item->label);
    background_init(&item->background);
    background_init(&item->selected_background);
    item->popup = calloc(1, sizeof(*item->popup));
    if (item->popup) popup_init(item->popup, item);
}

void bar_item_destroy(struct bar_item *item) {
    if (!item) return;
    free(item->name);
    text_destroy(&item->icon);
    text_destroy(&item->label);
    background_destroy(&item->background);
    background_destroy(&item->selected_background);
    if (item->graph) graph_destroy(item->graph);
    if (item->alias) alias_destroy(item->alias);
    if (item->slider) slider_destroy(item->slider);
    if (item->popup) {
        popup_destroy(item->popup);
        free(item->popup);
        item->popup = NULL;
    }
    free(item->script);
    free(item->click_script);
    free(item->format);
    free(item->app_menu_shortcut);
    item->app_menu_shortcut = NULL;
    free(item->scroll_values);
    free(item->windows);
    for (int i = 0; i < item->icon_strip_count; i++) {
        free(item->icon_strip[i]);
    }
    free(item->icon_strip);
}

static void text_clone(struct text *dst, const struct text *src) {
    text_init(dst);
    dst->color = src->color;
    dst->highlight_color = src->highlight_color;
    dst->has_shadow = src->has_shadow;
    dst->has_background = src->has_background;
    dst->background_color = src->background_color;
    dst->scroll_enabled = src->scroll_enabled;
    dst->scroll_duration = src->scroll_duration;
    dst->y_offset = src->y_offset;
    dst->x_offset = src->x_offset;
    dst->font = src->font;
    text_set_string(dst, src->string);
}

static void background_clone(struct background *dst, const struct background *src) {
    background_init(dst);
    dst->color = src->color;
    dst->border_color = src->border_color;
    dst->has_shadow = src->has_shadow;
    dst->shadow = src->shadow;
    dst->corner_radius = src->corner_radius;
    dst->border_width = src->border_width;
    dst->padding_left = src->padding_left;
    dst->padding_right = src->padding_right;
    dst->padding_top = src->padding_top;
    dst->padding_bottom = src->padding_bottom;
    dst->clip = src->clip;
    dst->y_offset = src->y_offset;
    dst->type = src->type;
    if (src->image) {
        dst->image = image_create(src->image->path);
        if (dst->image) {
            dst->image->border_width = src->image->border_width;
            dst->image->corner_radius = src->image->corner_radius;
            dst->image->border_color = src->image->border_color;
        }
    }
}

struct bar_item *bar_item_clone(const struct bar_item *src) {
    if (!src) return NULL;
    struct bar_item *dst = calloc(1, sizeof(*dst));
    if (!dst) return NULL;

    dst->type = src->type;
    dst->name = src->name ? strdup(src->name) : NULL;
    dst->format = src->format ? strdup(src->format) : NULL;
    dst->kind = src->kind;

    text_clone(&dst->icon, &src->icon);
    text_clone(&dst->label, &src->label);
    background_clone(&dst->background, &src->background);
    background_clone(&dst->selected_background, &src->selected_background);

    if (src->graph) {
        dst->graph = graph_create(src->graph->width, src->graph->height);
        dst->graph->fill_color = src->graph->fill_color;
        dst->graph->line_color = src->graph->line_color;
        dst->graph->max_points = src->graph->max_points;
        dst->graph->count = src->graph->count;
        if (dst->graph->data && src->graph->data) {
            memcpy(dst->graph->data, src->graph->data,
                   (size_t)dst->graph->count * sizeof(float));
        }
    }

    if (src->alias) {
        dst->alias = alias_create();
        if (dst->alias) {
            dst->alias->width = src->alias->width;
            dst->alias->height = src->alias->height;
            dst->alias->corner_radius = src->alias->corner_radius;
            dst->alias->disable_shadows = src->alias->disable_shadows;
            dst->alias->update_frequency = src->alias->update_frequency;
            dst->alias->inverse = src->alias->inverse;
            dst->alias->background_color = src->alias->background_color;
            alias_set_target(dst->alias, src->alias->owner, src->alias->name);
            alias_set_bundle_id(dst->alias, src->alias->bundle_id);
            alias_set_target_pid(dst->alias, src->alias->target_pid);
        }
    }

    dst->position = src->position;
    dst->associated_space = src->associated_space;
    dst->associated_display = src->associated_display;
    dst->associated_bar = src->associated_bar;
    dst->update_mask = src->update_mask;

    dst->space_id = src->space_id;
    dst->selected = src->selected;
    if (src->icon_strip && src->icon_strip_count > 0) {
        dst->icon_strip = malloc(sizeof(char *) * (size_t)src->icon_strip_count);
        if (dst->icon_strip) {
            for (int i = 0; i < src->icon_strip_count; i++) {
                dst->icon_strip[i] = src->icon_strip[i]
                                     ? strdup(src->icon_strip[i]) : NULL;
            }
            dst->icon_strip_count = src->icon_strip_count;
        }
    }

    dst->script = src->script ? strdup(src->script) : NULL;
    dst->click_script = src->click_script ? strdup(src->click_script) : NULL;

    dst->hidden = src->hidden;
    dst->click_enabled = src->click_enabled;
    dst->scroll_enabled = src->scroll_enabled;
    dst->scroll_sensitivity = src->scroll_sensitivity;
    if (src->scroll_values && src->scroll_value_count > 0) {
        dst->scroll_values = malloc(sizeof(float) * (size_t)src->scroll_value_count);
        if (dst->scroll_values)
            memcpy(dst->scroll_values, src->scroll_values,
                   sizeof(float) * (size_t)src->scroll_value_count);
        dst->scroll_value_count = src->scroll_value_count;
    }
    dst->scroll_index = src->scroll_index;
    dst->y_offset = src->y_offset;
    dst->padding_left = src->padding_left;
    dst->padding_right = src->padding_right;
    dst->label_x_offset = src->label_x_offset;
    dst->icon_x_offset = src->icon_x_offset;
    dst->update_interval = src->update_interval;
    dst->index = src->index;
    dst->app_menu_first = src->app_menu_first;
    dst->app_menu_slot = src->app_menu_slot;
    dst->app_menu_row_w = src->app_menu_row_w;
    dst->app_menu_shortcut = src->app_menu_shortcut
                                 ? strdup(src->app_menu_shortcut) : NULL;

    if (src->popup) {
        dst->popup = calloc(1, sizeof(*dst->popup));
        if (dst->popup) {
            popup_init(dst->popup, dst);
            dst->popup->cell_size = src->popup->cell_size;
            dst->popup->y_offset = src->popup->y_offset;
            dst->popup->background = src->popup->background;
            dst->popup->item_background = src->popup->item_background;
            /* deep-copy the popup item list */
            for (int i = 0; i < src->popup->item_count; i++) {
                struct bar_item *child = bar_item_clone(src->popup->items[i]);
                if (child) popup_add_item(dst->popup, child);
            }
        }
    }

    return dst;
}

struct window *bar_item_get_window(struct bar_item *item, int adid) {
    if (!item || item->window_count == 0 || !item->windows) return NULL;
    for (int i = 0; i < item->window_count; i++) {
        if (item->windows[i]->id == 0) continue;
        if (adid == -1 || item->windows[i]->parent /* popup child */) continue;
        return item->windows[i];
    }
    return item->windows[0];
}

/* ---- app menus ----------------------------------------------------------
   omabar covers the native menu bar, so the frontmost app's own menus have
   to be re-hosted in ours. They arrive through the Accessibility API (see
   app_menus.m) and are split across three item kinds so a theme can place
   each part independently:

     app_logo    the system Apple menu, drawn as the Apple symbol
     front_app   the app's own menu, drawn as the app name (an existing kind)
     app_menus   the app-provided menus -- "File", "Edit", ... -- as one row

   app_menus renders every menu it is handed as a separate clickable slot
   inside a single item, exactly the way a space strip renders one chip per
   space. That leaves the bar's layout code untouched and lets the slot count
   change with the front app, which is the whole point: Chrome and Finder do
   not offer the same menus. */

#define APP_MENU_PAD 8.0f
#define APP_MENU_SHORTCUT_GAP 16.0f

/* Slot widths for the menus this item renders, measured with the label font.
   Drawing and hit testing both go through this, so they cannot drift apart. */
static int bar_item_app_menus_slots(struct bar_item *item, float *widths) {
    int total = app_menus_refresh();
    int first = item->app_menu_first;
    if (total <= first) return 0;

    int count = total - first;
    if (count > OMABAR_MAX_APP_MENUS) count = OMABAR_MAX_APP_MENUS;
    for (int i = 0; i < count; i++) {
        const char *title = app_menus_title(first + i);
        widths[i] = text_measure(item->label.font, title) + APP_MENU_PAD * 2.0f;
    }
    return count;
}

static float bar_item_app_menus_width(struct bar_item *item) {
    float widths[OMABAR_MAX_APP_MENUS];
    int count = bar_item_app_menus_slots(item, widths);
    float total = 0.0f;
    for (int i = 0; i < count; i++) total += widths[i];
    return total;
}

/* The Apple symbol is a template image, so it is drawn pre-tinted: the tint is
   baked into a cached image by app_menus_apple_logo_tinted, which keeps the
   per-frame cost down to a single draw. */
static void bar_item_app_logo_draw(struct bar_item *item, CGContextRef ctx,
                                   CGRect frame) {
    
    CGImageRef image = app_menus_apple_logo_tinted(
        item->label.color.r, item->label.color.g, item->label.color.b,
        item->label.color.a);
    if (!image) {  return; }

    float size = item->label.font ? (float)item->label.font->size : 14.0f;
    if (size <= 0.0f) size = 14.0f;
    size *= 1.15f; /* the logo reads small at exactly text size */

    CGRect box = CGRectMake(frame.origin.x + (frame.size.width - size) / 2.0f,
                            frame.origin.y + (frame.size.height - size) / 2.0f,
                            size, size);

    CGContextSaveGState(ctx);
    CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
    CGContextDrawImage(ctx, box, image);
    CGContextRestoreGState(ctx);
}

/* A slot under the click, or -1. *offset receives the slot's left edge. */
static int bar_item_app_menus_hit(struct bar_item *item, CGPoint point,
                                  float *offset) {
    float widths[OMABAR_MAX_APP_MENUS];
    int count = bar_item_app_menus_slots(item, widths);
    float x = 0.0f;

    for (int i = 0; i < count; i++) {
        if (point.x >= x && point.x < x + widths[i]) {
            if (offset) *offset = x;
            return item->app_menu_first + i;
        }
        x += widths[i];
    }
    return -1;
}

static void bar_item_app_menus_draw(struct bar_item *item, CGContextRef ctx,
                                    CGRect frame) {
    float widths[OMABAR_MAX_APP_MENUS];
    int count = bar_item_app_menus_slots(item, widths);
    float h = item->label.line.ascent + item->label.line.descent;
    float x = 0.0f;

    for (int i = 0; i < count; i++) {
        const char *title = app_menus_title(item->app_menu_first + i);
        
        /* The label is re-pointed at each title in turn, the same way the
           space strip re-points item->icon per chip. This kind's width comes
           from bar_item_app_menus_width rather than item->label.line.width, so
           that re-pointing cannot make the row's geometry depend on whichever
           title was drawn last. */
        text_set_string(&item->label, title);
        float w = text_measure(item->label.font, title);
        CGRect text_frame = CGRectMake(x + (widths[i] - w) / 2.0f,
                                       (frame.size.height - h) / 2.0f, w, h);
        text_draw(&item->label, ctx, text_frame);
        x += widths[i];
    }
}

/* Fill the host item's popup with a menu's real leaves and open it.

   The popup is rebuilt on every open: leaves come and go as the app's state
   changes ("Forward" greys out, a "Sign In" item appears), and rebuilding is
   the only way to be sure the list matches what the app itself would show. */
static void bar_item_app_menu_open(struct bar_item *item, int menu) {
    int leaves = app_menus_load_items(menu);
    
    if (leaves <= 0) return;

    if (!item->popup) {
        item->popup = calloc(1, sizeof(*item->popup));
        if (!item->popup) return;
    } else {
        /* popup_destroy frees the item array but not the items in it */
        for (int i = 0; i < item->popup->item_count; i++)
            bar_item_destroy(item->popup->items[i]);
        popup_destroy(item->popup);
    }
    popup_init(item->popup, item);

    /* One width for every row, so the shortcuts line up in a column and the
       popup takes its width from a single row length. */
    float row = 0.0f;
    for (int i = 0; i < leaves; i++) {
        if (app_menus_item_is_separator(i)) continue;
        float w = text_measure(item->label.font, app_menus_item_title(i));
        const char *sc = app_menus_item_shortcut(i);
        if (sc && sc[0]) w += APP_MENU_SHORTCUT_GAP
                            + text_measure(item->label.font, sc);
        if (w > row) row = w;
    }
    if (row <= 0.0f) row = text_measure(item->label.font, " ");

    for (int i = 0; i < leaves; i++) {
        struct bar_item *leaf = calloc(1, sizeof(*leaf));
        if (!leaf) break;
        bar_item_init(leaf);

        leaf->type = BAR_ITEM;
        leaf->kind = BAR_KIND_APP_MENU_ITEM;
        leaf->position = item->position;
        leaf->padding_left = 4;
        leaf->padding_right = 4;
        leaf->app_menu_slot = i;
        leaf->app_menu_row_w = row;
        leaf->app_menu_host = item;
        leaf->app_menu_shortcut = strdup(app_menus_item_shortcut(i));

        /* the font object is owned by the config, not by the item, so the
           popup can borrow the host's without a second copy to free */
        bar_item_set_label_font(leaf, item->label.font);
        bar_item_set_label_color(leaf, item->label.color);
        if (!app_menus_item_enabled(i)) {
            bar_item_set_label_color(leaf,
                                     color_with_alpha(item->label.color, 0.4f));
        }
        text_set_string(&leaf->label, app_menus_item_title(i));
        leaf->click_enabled = !app_menus_item_is_separator(i)
                              && app_menus_item_enabled(i);

        leaf->app_menu_menu = menu;
        popup_add_item(item->popup, leaf);
    }

    popup_open(item->popup);
}

/* A menu re-read (app switched, or the front-app event fired) clears the
   shared leaf cache, which would leave an already-open popup blank. Reload on
   demand instead: the first draw or click after a re-read pays one AX read
   and the cache stays warm for every frame after that. */
static void bar_item_app_menu_ensure_items(struct bar_item *item) {
    if (!item || item->app_menu_menu < 0) return;
    if (app_menus_items_menu() == item->app_menu_menu
        && app_menus_item_count() > 0)
        return;
    app_menus_load_items(item->app_menu_menu);
}

/* A leaf draws its title flush left and its shortcut flush right in the row
   width the popup settled on, which is what lines the column up. */
static void bar_item_app_menu_item_draw(struct bar_item *item, CGContextRef ctx,
                                        CGRect frame) {
    bar_item_app_menu_ensure_items(item);
    float h = item->label.line.ascent + item->label.line.descent;
    float y = (frame.size.height - h) / 2.0f;

    if (app_menus_item_is_separator(item->app_menu_slot)) {
        /* a hairline, the way AppKit separates groups of items */
        float mid = frame.origin.y + frame.size.height / 2.0f;
        CGContextSaveGState(ctx);
        CGContextSetLineWidth(ctx, 1.0f);
        CGContextSetRGBStrokeColor(ctx, item->label.color.r,
                                    item->label.color.g, item->label.color.b,
                                    item->label.color.a * 0.35f);
        CGContextMoveToPoint(ctx, frame.origin.x, mid);
        CGContextAddLineToPoint(ctx, frame.origin.x + frame.size.width, mid);
        CGContextStrokePath(ctx);
        CGContextRestoreGState(ctx);
        return;
    }

    CGRect title_frame = CGRectMake(frame.origin.x, y,
                                    item->label.line.width, h);
    text_draw(&item->label, ctx, title_frame);

    const char *sc = item->app_menu_shortcut;
    if (sc && sc[0]) {
        float sw = text_measure(item->label.font, sc);
        CGRect sc_frame = CGRectMake(frame.origin.x + item->app_menu_row_w - sw,
                                     y, sw, h);
        struct text tmp;
        text_init(&tmp);
        text_set_font(&tmp, item->label.font);
        text_set_color(&tmp, item->label.color);
        text_set_string(&tmp, sc);
        text_draw(&tmp, ctx, sc_frame);
        text_destroy(&tmp);
    }
}

float bar_item_get_content_length(struct bar_item *item) {
    if (!item) return 0;

    /* a space strip is as wide as its whole chip row, padding included */
    if (item->type == BAR_COMPONENT_SPACE)
        return bar_item_space_row_width(item);

    /* the menu row is as wide as all of its slots; the logo is a square */
    if (item->kind == BAR_KIND_APP_MENUS)
        return bar_item_app_menus_width(item);

    if (item->kind == BAR_KIND_APP_LOGO) {
        float size = item->label.font ? (float)item->label.font->size : 14.0f;
        if (size <= 0.0f) size = 14.0f;
        return size * 1.15f;
    }

    /* every popup row shares one width so the shortcut column lines up */
    if (item->kind == BAR_KIND_APP_MENU_ITEM) return item->app_menu_row_w;

    float icon_w = bar_item_icon_width(item);
    if (icon_w <= 0.0f) icon_w = item->icon.line.width;
    float length = icon_w;
    if (item->graph) length += (float)item->graph->width;
    if (item->alias) {
        if (item->alias->image)
            length += (float)CGImageGetWidth(item->alias->image);
        else
            length += (float)item->alias->width;
    }
    if (item->slider) length += (float)item->slider->width;
    length += item->label.line.width;
    return length;
}

float bar_item_get_length(struct bar_item *item) {
    if (!item) return 0;
    return bar_item_get_content_length(item)
           + item->padding_left + item->padding_right;
}

void bar_item_update(struct bar_item *item, const char *sender, const char *info);

static const char *bar_item_button_name(uint32_t button) {
    switch (button) {
        case 0: return "left";
        case 1: return "right";
        case 2: return "middle";
        case 3: return "back";
        case 4: return "forward";
        default: return "unknown";
    }
}

static struct env_vars bar_item_make_env(struct bar_item *item,
                                         const char *sender,
                                         const char *info,
                                         const char *button,
                                         const char *modifier,
                                         const char *scroll_delta) {
    struct env_vars env;
    env_vars_init(&env);
    env_vars_set(&env, "NAME", item->name);
    env_vars_set(&env, "SENDER", sender ? sender : "routine");
    if (info) env_vars_set(&env, "INFO", info);

    if (item->type == BAR_COMPONENT_SPACE) {
        char sid[32], selected[4];
        snprintf(sid, sizeof(sid), "%llu",
                 (unsigned long long)item->space_id);
        snprintf(selected, sizeof(selected), "%d", !!item->selected);
        env_vars_set(&env, "SID", sid);
        env_vars_set(&env, "SELECTED", selected);
    }

    if (item->associated_display) {
        char did[32];
        snprintf(did, sizeof(did), "%u",
                 get_set_bit_position(item->associated_display));
        env_vars_set(&env, "DID", did);
    }

    if (button) env_vars_set(&env, "BUTTON", button);
    if (modifier) env_vars_set(&env, "MODIFIER", modifier);
    if (scroll_delta) env_vars_set(&env, "SCROLL_DELTA", scroll_delta);

    for (int i = 0; i < g_env_vars.count; i++) {
        if (g_env_vars.vars[i].key)
            env_vars_set(&env, g_env_vars.vars[i].key,
                         g_env_vars.vars[i].value);
    }
    return env;
}

/* Send an output line back to the daemon as an IPC command. */
static void bar_item_dispatch_script_output(const char *line) {
    if (!line || !*line) return;

    struct event *event = malloc(sizeof(*event));
    if (!event) return;
    event->type = EVENT_DAEMON_MESSAGE;
    event->data = strdup(line);
    event_post(event);
    free(event);
}

static void bar_item_run_script(struct bar_item *item, struct env_vars *env) {
    if (!item || !item->script || !*item->script) return;

    if (item->mach_helper) {
        uint32_t len = 0;
        char *message = env_vars_copy_serialized_representation(env, &len);
        if (message) {
            mach_send_message((mach_port_t)(uintptr_t)item->mach_helper,
                              message, len, false);
            free(message);
        }
        return;
    }

    /* When the script's stdout already carries our own event loop thread,
       re-dispatching must not block; run with output capture so any
       commands are parsed back into the daemon. */
    char *output = NULL;
    if (fork_exec_output(item->script, env, &output)) {
        char *save = NULL;
        for (char *line = output ? strtok_r(output, "\n", &save) : NULL;
             line; line = strtok_r(NULL, "\n", &save)) {
            bar_item_dispatch_script_output(line);
        }
    }
    free(output);
}

static void bar_item_refresh(struct bar_item *item) {
    bar_item_calculate_bounds(item);
    if (item->window_count > 0)
        g_bar_manager.bar_needs_update = true;
}

void bar_item_update(struct bar_item *item, const char *sender, const char *info) {
    if (!item) return;

    /* The menu cache is keyed on the front app, so an app switch has to force a
       re-read before the new app's menus are measured. */
    if (info && strcmp(info, "front_app_switched") == 0
        && (item->kind == BAR_KIND_APP_LOGO || item->kind == BAR_KIND_APP_MENUS))
        app_menus_invalidate();

    /* built-in kinds render their own value; scripts (if any) take over */
    bar_item_apply_builtin_content(item);

    if (item->script) {
        struct env_vars env = bar_item_make_env(item, sender, info,
                                                NULL, NULL, NULL);
        bar_item_run_script(item, &env);
        env_vars_destroy(&env);
    }

    /* component refreshes: aliases capture on app switches / routine ticks,
       graphs advance their live data stream on the animation tick. */
    if (item->alias) {
        if (!item->alias->permission)
            alias_request_permission(item->alias);
        bool forced = sender
            && (strstr(sender, "front_app") || strstr(sender, "forced"));
        if (alias_update(item->alias, forced))
            bar_item_refresh(item);
    }
    else if (item->graph && item->graph->data_source
             && item->graph->data_source(item)) {
        bar_item_refresh(item);
    }

    bar_item_calculate_bounds(item);
    if (item->window_count > 0)
        g_bar_manager.bar_needs_update = true;
}

void bar_item_on_click(struct bar_item *item, uint32_t button,
                       uint32_t modifier, CGPoint point) {
    if (!item) return;

    if (item->type == BAR_COMPONENT_SPACE) {
        bar_item_space_clicked(item, point);
        return;
    }

    /* the Apple menu and the app's menus both open the menu under the click;
       the leaves inside a popup perform their command instead */
    if (item->kind == BAR_KIND_APP_LOGO) {
        if (button == 0) bar_item_app_menu_open(item, OMABAR_APP_MENU_APPLE);
        return;
    }

    if (item->kind == BAR_KIND_APP_MENUS) {
        if (button == 0) {
            int menu = bar_item_app_menus_hit(item, point, NULL);
            if (menu >= 0) bar_item_app_menu_open(item, menu);
        }
        return;
    }

    if (item->kind == BAR_KIND_APP_MENU_ITEM)


    if (item->kind == BAR_KIND_APP_MENU_ITEM) {
        bar_item_app_menu_ensure_items(item);
        /* check the live state, not the one captured when the popup was built:
           the app may have greyed the item out since */
        if (button != 0 || !item->click_enabled
            || app_menus_item_is_separator(item->app_menu_slot)
            || !app_menus_item_enabled(item->app_menu_slot))
            return;
        int pr = app_menus_press(item->app_menu_slot);
        
        if (pr == 0) {
            /* the command has run against the app, so get the menu out of the
               way instead of leaving it floating over the result */
            struct bar_item *host = item->app_menu_host;
            if (host && host->popup && host->popup->is_open)
                popup_close(host->popup);
        }
        return;
    }

    /* a host item with a popup toggles it on click. Buttons are numbered the
       way CGEvent numbers them (0 left, 1 right, 2 middle), so a left click is
       0 here — testing for 1 made popups open on right-click only. */
    if (item->popup && popup_has_items(item->popup)
        && item->popup->drawing && button == 0) {
        popup_toggle(item->popup);
        bar_item_refresh(item);
    }

    if (item->slider) {
        CGRect frame = bar_item_slider_frame(item);
        if (slider_hit_test(frame, point)) {
            if (slider_handle_drag(item->slider, point, frame)) {
                bar_item_refresh(item);
            }
            slider_cancel_drag(item->slider);
        }
    }

    struct env_vars env = bar_item_make_env(item, "mouse.clicked",
                                            NULL, bar_item_button_name(button),
                                            "none", NULL);

    if (item->click_script) {
        fork_exec(item->click_script, &env);
    }
    if (item->script && (item->update_mask & UPDATE_MOUSE_CLICKED)) {
        bar_item_run_script(item, &env);
    }

    env_vars_destroy(&env);
}

void bar_item_on_drag(struct bar_item *item, CGPoint point) {
    if (!item || !item->slider) return;
    if (!item->slider->is_dragged) return;

    CGRect frame = bar_item_slider_frame(item);
    if (slider_handle_drag(item->slider, point, frame)) {
        char percentage[16];
        snprintf(percentage, sizeof(percentage), "%d",
                 (int)lround(((item->slider->value - item->slider->min)
                              / (item->slider->max - item->slider->min)) * 100.0));
        struct env_vars env = bar_item_make_env(item, "mouse.dragged",
                                                percentage, "none", "none", NULL);
        if (item->script && (item->update_mask & UPDATE_MOUSE_DRAGGED))
            bar_item_run_script(item, &env);
        env_vars_destroy(&env);

        bar_item_refresh(item);
    }
}

void bar_item_cancel_drag(struct bar_item *item) {
    if (!item || !item->slider) return;

    char percentage[16];
    if (item->slider->max > item->slider->min) {
        snprintf(percentage, sizeof(percentage), "%d",
                 (int)lround(((item->slider->value - item->slider->min)
                              / (item->slider->max - item->slider->min)) * 100.0));
    } else {
        snprintf(percentage, sizeof(percentage), "%d", 0);
    }

    struct env_vars env = bar_item_make_env(item, "mouse.clicked",
                                            percentage, "none", "none", NULL);
    if (item->click_script) {
        fork_exec(item->click_script, &env);
    }
    if (item->script && (item->update_mask & UPDATE_MOUSE_CLICKED)) {
        bar_item_run_script(item, &env);
    }
    env_vars_destroy(&env);

    slider_cancel_drag(item->slider);
    bar_item_refresh(item);
}

void bar_item_on_scroll(struct bar_item *item, int scroll_delta,
                        uint32_t modifier) {
    if (!item) return;
    (void)modifier;

    if (item->scroll_enabled && item->scroll_sensitivity > 0.0f) {
        float target_value;

        /* advance through the scroll_values list (wrap-around); the bump
           glides to the newly selected list value */
        if (item->scroll_values && item->scroll_value_count > 0) {
            item->scroll_index =
                (item->scroll_index + scroll_delta) % item->scroll_value_count;
            if (item->scroll_index < 0)
                item->scroll_index += item->scroll_value_count;
            target_value = item->scroll_values[item->scroll_index];
        } else {
            target_value = (float)scroll_delta * item->scroll_sensitivity;
        }

        /* smooth scrub: bump scroll_offset out, then ease it back to zero */
        struct animation *out = calloc(1, sizeof(struct animation));
        struct animation *in  = calloc(1, sizeof(struct animation));
        if (out && in) {
            out->target = &item->scroll_offset;
            out->type = ANIMATION_FLOAT;
            out->function = ANIMATION_EASE_OUT;
            out->initial = item->scroll_offset;
            out->final = target_value;
            out->duration = 0.18;

            in->target = &item->scroll_offset;
            in->type = ANIMATION_FLOAT;
            in->function = ANIMATION_EASE_OUT;
            in->initial = target_value; /* overwritten when chained */
            in->final = 0.0f;
            in->duration = 0.25;

            out->next = in;
            animation_run(&g_bar_manager.animator, out);
        } else {
            free(out);
            free(in);
        }
    }

    if (item->update_mask & UPDATE_MOUSE_SCROLLED) {
        char delta[16];
        snprintf(delta, sizeof(delta), "%d", scroll_delta);
        struct env_vars env = bar_item_make_env(item, "mouse.scrolled",
                                                NULL, NULL, "none", delta);
        if (item->script) bar_item_run_script(item, &env);
        env_vars_destroy(&env);
    }
}

void bar_item_mouse_entered(struct bar_item *item) {
    if (!item || item->mouse_over) return;
    item->mouse_over = true;
    if (item->update_mask & UPDATE_MOUSE_ENTERED)
        bar_item_update(item, "mouse.entered", NULL);
}

void bar_item_mouse_exited(struct bar_item *item) {
    if (!item || !item->mouse_over) return;
    item->mouse_over = false;
    if (item->update_mask & UPDATE_MOUSE_EXITED)
        bar_item_update(item, "mouse.exited", NULL);
}

CGRect bar_item_slider_frame(struct bar_item *item) {
    if (!item || !item->slider) return CGRectZero;
    /* slider is drawn after the icon run: padding + icon_x_offset + icon width */
    float x = (float)item->padding_left + (float)item->icon_x_offset
              + item->icon.line.width;
    return CGRectMake(x, (float)item->y_offset,
                      (float)item->slider->width,
                      (float)item->slider->height);
}

bool bar_item_has_slider(struct bar_item *item) {
    return item && item->slider;
}

void bar_item_set_slider(struct bar_item *item, double min, double max,
                         double value) {
    if (!item) return;
    if (!item->slider) item->slider = slider_create();
    if (!item->slider) return;
    slider_set_range(item->slider, value, min, max);
    bar_item_calculate_bounds(item);
    if (item->window_count > 0)
        g_bar_manager.bar_needs_update = true;
}

double bar_item_slider_value(struct bar_item *item) {
    if (!item || !item->slider) return 0.0;
    return item->slider->value;
}

CGRect bar_item_calculate_bounds(struct bar_item *item) {
    if (!item) return CGRectNull;
    /* content flow: icon -> component -> label, laid out left to right */
    float icon_w = bar_item_icon_width(item);
    if (icon_w <= 0.0f) icon_w = item->icon.line.width;
    float label_w = item->label.line.width;

    float comp_w = 0;
    if (item->graph) comp_w = (float)item->graph->width;
    else if (item->alias) {
        comp_w = item->alias->image
                 ? (float)CGImageGetWidth(item->alias->image)
                 : (float)item->alias->width;
    }
    else if (item->slider) comp_w = (float)item->slider->width;

    /* Offsets are applied by the draw path through a running cursor that
       already starts at padding_left, so the text structs must keep their
       own offsets untouched here. Folding them in again would compound on
       every relayout and push content out of its own background. */

    float content_w = icon_w + comp_w + label_w;
    float total_w = item->padding_left + content_w + item->padding_right;
    float total_h = (float)BAR_ITEM_ROW_HEIGHT;

    return CGRectMake(0, (float)item->y_offset, total_w, total_h);
}

void bar_item_draw(struct bar_item *item, struct bar *bar, CGContextRef ctx) {
    (void)bar;
    if (!item || !ctx || item->hidden) return;

    /* space component: a row of chips, one per space on this display */
    if (item->type == BAR_COMPONENT_SPACE) {
        float sw = bar_item_space_row_width(item);
        float sh = (float)BAR_ITEM_ROW_HEIGHT;
        CGRect sframe = CGRectMake(0, (float)item->y_offset, sw, sh);
        bar_item_space_draw(item, bar, ctx, sframe);
        return;
    }

    /* the app's menus draw their own slots, and the leaf kind is drawn by the
       popup that hosts it */
    if (item->kind == BAR_KIND_APP_LOGO) {
        float size = item->label.font ? (float)item->label.font->size : 14.0f;
        if (size <= 0.0f) size = 14.0f;
        /* full-height frame: the draw helper centres the square in the row,
           so only the horizontal centring is ours to do */
        float side = size * 1.15f;
        CGRect frame = CGRectMake((item->padding_left - item->padding_right) / 2.0f,
                                  (float)item->y_offset, side,
                                  (float)BAR_ITEM_ROW_HEIGHT);
        bar_item_app_logo_draw(item, ctx, frame);
        return;
    }

    if (item->kind == BAR_KIND_APP_MENUS) {
        float w = bar_item_app_menus_width(item);
        CGRect frame = CGRectMake(0, (float)item->y_offset, w,
                                  (float)BAR_ITEM_ROW_HEIGHT);
        bar_item_app_menus_draw(item, ctx, frame);
        return;
    }

    if (item->kind == BAR_KIND_APP_MENU_ITEM) {
        float w = item->padding_left + item->app_menu_row_w
                  + (float)item->padding_right;
        CGRect frame = CGRectMake(0, (float)item->y_offset, w,
                                  (float)BAR_ITEM_ROW_HEIGHT);
        bar_item_app_menu_item_draw(item, ctx, frame);
        return;
    }

    float content = bar_item_get_content_length(item);
    float total_w = item->padding_left + content + item->padding_right;
    float total_h = (float)BAR_ITEM_ROW_HEIGHT;
    CGRect frame = CGRectMake(0, (float)item->y_offset, total_w, total_h);

    /* background layer */
    if (item->background.type > 0 || item->background.color.a > 0
        || item->background.has_shadow) {
        background_draw(&item->background, ctx, frame);
    }

    /* a divider carries nothing but its hairline */
    if (item->kind == BAR_KIND_DIVIDER) {
        bar_item_draw_divider(item, ctx, frame);
        return;
    }

    /* icon: either a text glyph or a built-in vector icon */
    float cursor = item->padding_left + item->icon_x_offset;
    float vec_w = bar_item_draw_vector_icon(item, ctx, frame);
    if (vec_w <= 0.0f) {
        float icon_h = item->icon.line.ascent + item->icon.line.descent;
        float y = (total_h - icon_h) / 2.0f;
        CGRect icon_frame = CGRectMake(cursor, y + (float)item->y_offset,
                                       item->icon.line.width, icon_h);
        text_draw(&item->icon, ctx, icon_frame);
        vec_w = item->icon.line.width;
    }
    cursor += vec_w;

    /* component (graph / alias / slider) */
    if (item->graph) {
        CGRect gframe = CGRectMake(cursor, (float)item->y_offset,
                                   item->graph->width, total_h);
        graph_draw(item->graph, ctx, gframe);
        cursor += item->graph->width;
    }
    else if (item->alias) {
        float aw = item->alias->image
                   ? (float)CGImageGetWidth(item->alias->image)
                   : (float)item->alias->width;
        float ah = item->alias->image
                   ? (float)CGImageGetHeight(item->alias->image)
                   : (float)item->alias->height;
        if (ah > total_h) ah = total_h;
        CGRect aframe = CGRectMake(cursor,
                                   (total_h - ah) / 2.0f + (float)item->y_offset,
                                   aw, ah);
        alias_draw(item->alias, ctx, aframe);
        cursor += aw;
    }
    else if (item->slider) {
        CGRect sframe = CGRectMake(cursor, (float)item->y_offset,
                                   item->slider->width, item->slider->height);
        slider_draw(item->slider, ctx, sframe);
        cursor += item->slider->width;
    }

    /* label */
    cursor += item->label_x_offset;
    float label_h = item->label.line.ascent + item->label.line.descent;
    CGRect label_frame = CGRectMake(cursor,
                                    (total_h - label_h) / 2.0f + (float)item->y_offset,
                                    item->label.line.width, label_h);
    text_draw(&item->label, ctx, label_frame);
}

static void bar_item_needs_refresh(struct bar_item *item, bool changed) {
    if (!changed || !item) return;
    bar_item_calculate_bounds(item);
    if (item->window_count > 0)
        g_bar_manager.bar_needs_update = true;
}

void bar_item_set_name(struct bar_item *item, const char *name) {
    if (!item || !name) return;
    if (item->name && string_equals(item->name, name)) return;
    free(item->name);
    item->name = strdup(name);
    bar_item_needs_refresh(item, true);
}

void bar_item_set_icon(struct bar_item *item, const char *string) {
    if (!item || !string) return;
    if (item->icon.string && string_equals(item->icon.string, string)) return;
    text_set_string(&item->icon, string);
    bar_item_needs_refresh(item, true);
}

void bar_item_set_label(struct bar_item *item, const char *string) {
    if (!item || !string) return;
    if (item->label.string && string_equals(item->label.string, string)) return;
    text_set_string(&item->label, string);
    bar_item_needs_refresh(item, true);
}

void bar_item_set_icon_font(struct bar_item *item, struct font *font) {
    if (!item || !font) return;
    text_set_font(&item->icon, font);
    bar_item_needs_refresh(item, true);
}

void bar_item_set_label_font(struct bar_item *item, struct font *font) {
    if (!item || !font) return;
    text_set_font(&item->label, font);
    bar_item_needs_refresh(item, true);
}

void bar_item_set_icon_color(struct bar_item *item, struct color color) {
    if (!item) return;
    if (color_equal(item->icon.color, color)) return;
    text_set_color(&item->icon, color);
    bar_item_needs_refresh(item, true);
}

void bar_item_set_label_color(struct bar_item *item, struct color color) {
    if (!item) return;
    if (color_equal(item->label.color, color)) return;
    text_set_color(&item->label, color);
    bar_item_needs_refresh(item, true);
}

void bar_item_set_background_color(struct bar_item *item, struct color color) {
    if (!item) return;
    if (color_equal(item->background.color, color)) return;
    item->background.color = color;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_background_border_color(struct bar_item *item, struct color color) {
    if (!item) return;
    if (color_equal(item->background.border_color, color)) return;
    item->background.border_color = color;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_background_corner_radius(struct bar_item *item, int radius) {
    if (!item || radius < 0) return;
    if (item->background.corner_radius == radius) return;
    item->background.corner_radius = radius;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_background_border_width(struct bar_item *item, int width) {
    if (!item || width < 0) return;
    if (item->background.border_width == width) return;
    item->background.border_width = width;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_background_height(struct bar_item *item, int height) {
    if (!item) return;
    item->background.padding_top = height;
    item->background.padding_bottom = 0;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_shadow(struct bar_item *item, int enabled) {
    if (!item) return;
    if (item->background.has_shadow == enabled) return;
    item->background.has_shadow = enabled;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_position(struct bar_item *item, enum bar_item_position position) {
    if (!item) return;
    if (item->position == position) return;
    item->position = position;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_update_mask(struct bar_item *item, uint64_t mask) {
    if (!item) return;
    item->update_mask = mask;
    bar_item_needs_refresh(item, true);
}

void bar_item_event_subscribe(struct bar_item *item, const char *event_name) {
    if (!item || !event_name) return;

    uint64_t mask = custom_events_get_mask(&g_bar_manager.custom_events,
                                           event_name);
    if (!mask) return;

    item->update_mask |= mask;

    /* ensure the source for system events is active (idempotent) */
    if (mask & UPDATE_VOLUME_CHANGED)
        volume_begin();
    if (mask & UPDATE_POWER_CHANGED)
        power_begin();
    if (mask & UPDATE_WIFI_CHANGED)
        wifi_begin();
    if (mask & UPDATE_MEDIA_CHANGED)
        media_begin();
    if (mask & UPDATE_BRIGHTNESS_CHANGED)
        display_brightness_begin();
    if (mask & (UPDATE_FRONT_APP_SWITCHED | UPDATE_WINDOW_FOCUSED))
        app_windows_begin();

    bar_item_needs_refresh(item, false);
}

void bar_item_event_unsubscribe(struct bar_item *item, const char *event_name) {
    if (!item || !event_name) return;

    uint64_t mask = custom_events_get_mask(&g_bar_manager.custom_events,
                                           event_name);
    if (!mask) return;

    item->update_mask &= ~mask;
    bar_item_needs_refresh(item, false);
}

void bar_item_set_script(struct bar_item *item, const char *script) {
    if (!item) return;
    if (item->script && string_equals(item->script, script)) return;
    free(item->script);
    item->script = script ? strdup(script) : NULL;
    bar_item_needs_refresh(item, false);
}

void bar_item_set_click_script(struct bar_item *item, const char *script) {
    if (!item) return;
    if (item->click_script && string_equals(item->click_script, script)) return;
    free(item->click_script);
    item->click_script = script ? strdup(script) : NULL;
}

void bar_item_set_hidden(struct bar_item *item, int hidden) {
    if (!item) return;
    if (item->hidden == hidden) return;
    item->hidden = hidden;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_click_enabled(struct bar_item *item, int enabled) {
    if (!item) return;
    item->click_enabled = enabled;
}

void bar_item_set_scroll_enabled(struct bar_item *item, int enabled) {
    if (!item) return;
    item->scroll_enabled = enabled;
}

void bar_item_set_scroll_sensitivity(struct bar_item *item, float sensitivity) {
    if (!item) return;
    if (item->scroll_sensitivity == sensitivity) return;
    item->scroll_sensitivity = sensitivity;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_scroll_values(struct bar_item *item, float *values, int count) {
    if (!item) return;
    free(item->scroll_values);
    item->scroll_values = NULL;
    item->scroll_value_count = 0;
    item->scroll_index = 0;
    if (values && count > 0) {
        item->scroll_values = malloc(sizeof(float) * (size_t)count);
        if (item->scroll_values) {
            memcpy(item->scroll_values, values, sizeof(float) * (size_t)count);
            item->scroll_value_count = count;
        }
    }
    bar_item_needs_refresh(item, true);
}

void bar_item_set_y_offset(struct bar_item *item, int offset) {
    if (!item) return;
    if (item->y_offset == offset) return;
    item->y_offset = offset;
    bar_item_needs_refresh(item, true);
}

/* Keep alias items live: re-capture when the front app changes and on the
   routine scroll tick so menu bar changes (battery %, clocks…) are seen. */
static void bar_item_alias_enable_live_updates(struct bar_item *item) {
    if (!item || !item->alias) return;
    if (!item->alias->permission)
        alias_request_permission(item->alias);
    bar_item_event_subscribe(item, "front_app_switched");
    bar_item_event_subscribe(item, "scroll.tick");
}

void bar_item_set_alias_target(struct bar_item *item, const char *owner,
                               const char *name) {
    if (!item) return;
    if (!item->alias) item->alias = alias_create();
    if (!item->alias) return;
    alias_set_target(item->alias, owner, name);
    bar_item_alias_enable_live_updates(item);
    bar_item_needs_refresh(item, true);
}

void bar_item_set_alias_bundle_id(struct bar_item *item, const char *bundle_id) {
    if (!item) return;
    if (!item->alias) item->alias = alias_create();
    if (!item->alias) return;
    alias_set_bundle_id(item->alias, bundle_id);
    bar_item_alias_enable_live_updates(item);
    bar_item_needs_refresh(item, true);
}

void bar_item_set_alias_size(struct bar_item *item, int width, int height) {
    if (!item) return;
    if (!item->alias) item->alias = alias_create();
    if (!item->alias) return;
    alias_set_size(item->alias, width, height);
    bar_item_alias_enable_live_updates(item);
    bar_item_needs_refresh(item, true);
}

void bar_item_set_graph(struct bar_item *item, float width, float height) {
    if (!item) return;
    if (!item->graph) item->graph = graph_create(width, height);
    if (!item->graph) return;
    item->graph->width = width;
    item->graph->height = height;
    /* CVDisplayLink-driven redraw: the animation tick re-enters this item so
       its data source can push fresh values. */
    bar_item_event_subscribe(item, "scroll.tick");
    bar_item_needs_refresh(item, true);
}

void bar_item_graph_push(struct bar_item *item, float value) {
    if (!item || !item->graph) return;
    graph_push_value(item->graph, value);
    bar_item_needs_refresh(item, true);
}

void bar_item_graph_set_range(struct bar_item *item, float min, float max) {
    if (!item || !item->graph) return;
    graph_set_range(item->graph, min, max);
    bar_item_needs_refresh(item, true);
}

void bar_item_graph_set_data_source(struct bar_item *item,
                                    graph_data_source_t source) {
    if (!item || !item->graph) return;
    graph_set_data_source(item->graph, source);
}

void bar_item_set_padding(struct bar_item *item, int left, int right, int top, int bottom) {
    if (!item) return;
    if (left >= 0 && (item->padding_left != left || item->padding_right != right
                      || item->background.padding_top != top
                      || item->background.padding_bottom != bottom)) {
        item->padding_left = left;
        item->padding_right = right;
        item->background.padding_top = top;
        item->background.padding_bottom = bottom;
        bar_item_needs_refresh(item, true);
    }
}

void bar_item_set_label_x_offset(struct bar_item *item, int offset) {
    if (!item) return;
    if (item->label_x_offset == offset) return;
    item->label_x_offset = offset;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_icon_x_offset(struct bar_item *item, int offset) {
    if (!item) return;
    if (item->icon_x_offset == offset) return;
    item->icon_x_offset = offset;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_index(struct bar_item *item, int index) {
    if (!item) return;
    item->index = index;
}
void bar_item_set_type(struct bar_item *item, enum bar_item_type type) {
    if (!item) return;
    item->type = type;
    if (type == BAR_COMPONENT_SPACE)
        item->update_mask |= UPDATE_SPACE_CHANGED;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_icon_strip(struct bar_item *item, char **strip, int count) {
    if (!item) return;
    for (int i = 0; i < item->icon_strip_count; i++)
        free(item->icon_strip[i]);
    free(item->icon_strip);
    item->icon_strip = NULL;
    item->icon_strip_count = 0;

    if (!strip || count <= 0) return;

    item->icon_strip = malloc(sizeof(char *) * (size_t)count);
    if (!item->icon_strip) return;
    for (int i = 0; i < count; i++) {
        item->icon_strip[i] = strip[i] ? strdup(strip[i]) : NULL;
        if (item->icon_strip[i]) item->icon_strip_count++;
    }
    bar_item_needs_refresh(item, true);
}

void bar_item_set_space_id(struct bar_item *item, uint64_t sid) {
    if (!item) return;
    if (item->space_id == sid) return;
    item->space_id = sid;
    bar_item_needs_refresh(item, true);
}

void bar_item_set_selected(struct bar_item *item, int selected) {
    if (!item) return;
    if (item->selected == selected) return;
    item->selected = selected;
    bar_item_needs_refresh(item, true);
}

/* choose the icon string for the current space: icon_strip[sid-1] or fallback */
/* Glyph for the chip at `index` in the strip: an explicit space_id maps
   straight onto icon_strip, otherwise the chip's position in the row does. */
static const char *bar_item_space_icon(struct bar_item *item, int index) {
    if (!item->icon_strip || item->icon_strip_count <= 0) return NULL;
    int slot = item->space_id > 0 ? (int)item->space_id - 1 : index;
    if (slot < 0 || slot >= item->icon_strip_count) return NULL;
    return item->icon_strip[slot];
}

/* Width of a single chip: the measured glyph plus the item's inner padding.
   Every chip in the row shares one width, so it is measured from the widest
   label the row can show rather than from item->icon, which the draw loop
   re-points at each chip in turn. Reading item->icon.line.width here would
   make the strip's geometry depend on whichever label was drawn last. */
static float bar_item_space_chip_width(struct bar_item *item) {
    float widest = 0.0f;
    int count = item->space_count > 0 ? item->space_count : 1;
    char numbuf[16];

    for (int i = 0; i < count; i++) {
        if (bar_item_space_icon(item, i)) continue;  /* strip glyph: not text */
        snprintf(numbuf, sizeof(numbuf), "%d", i + 1);
        float w = text_measure(item->icon.font, numbuf);
        if (w > widest) widest = w;
    }

    if (widest <= 0.0f) widest = (float)item->icon.line.width;

    return widest
           + (float)item->padding_left
           + (float)item->padding_right;
}

/* Total width of the chip row, gaps included. */
float bar_item_space_row_width(struct bar_item *item) {
    if (!item) return 0;
    int count = item->space_count > 0 ? item->space_count : 1;
    float chip = bar_item_space_chip_width(item);
    if (count <= 1) return chip;
    return chip * (float)count + (float)(item->space_gap) * (float)(count - 1);
}

int bar_item_space_draw(struct bar_item *item, struct bar *bar, CGContextRef ctx, CGRect frame) {
    (void)bar;
    if (!item || !ctx || item->hidden) return 0;
    if (item->type != BAR_COMPONENT_SPACE) return 0;

    int count = item->space_count > 0 ? item->space_count : 1;
    float chip_w = bar_item_space_chip_width(item);
    float glyph_w = item->icon.line.width;
    float glyph_h = item->icon.line.ascent + item->icon.line.descent;

    /* selected highlight: prefer the configured selected_background, and
       fall back to a translucent white chip when none is set */
    struct background selected_bg = item->selected_background;
    if (selected_bg.color.a <= 0.0f && selected_bg.type <= 0
        && !selected_bg.has_shadow) {
        struct color fallback = { 1.0f, 1.0f, 1.0f, 0.35f, 1 };
        selected_bg.color = fallback;
    }
    bool has_sel_bg = selected_bg.color.a > 0.0f || selected_bg.type > 0
                      || selected_bg.has_shadow;
    bool has_bg = item->background.type > 0 || item->background.color.a > 0
                  || item->background.has_shadow;

    uint64_t current = bar ? bar->sid : item->space_id;

    for (int i = 0; i < count; i++) {
        uint64_t sid = item->space_ids[i];
        bool is_selected = (sid == current);

        CGRect chip = CGRectMake(frame.origin.x + (float)i * (chip_w
                                 + (float)item->space_gap),
                                 frame.origin.y, chip_w, frame.size.height);

        if (is_selected) {
            if (has_sel_bg) background_draw(&selected_bg, ctx, chip);
        } else if (has_bg) {
            background_draw(&item->background, ctx, chip);
        }

        /* label: strip glyph when configured, else the 1-based chip number */
        const char *glyph = bar_item_space_icon(item, i);
        char numbuf[16];
        if (!glyph) {
            snprintf(numbuf, sizeof(numbuf), "%d", i + 1);
            glyph = numbuf;
        }
        text_set_string(&item->icon, glyph);
        glyph_w = item->icon.line.width;
        glyph_h = item->icon.line.ascent + item->icon.line.descent;

        /* selected chips flip to highlight_color, e.g. dark glyphs on a
           solid light fill */
        text_set_highlighted(&item->icon, is_selected);

        CGRect glyph_frame = CGRectMake(
            chip.origin.x + (chip_w - glyph_w) / 2.0f,
            chip.origin.y + (frame.size.height - glyph_h) / 2.0f,
            glyph_w, glyph_h);
        text_draw(&item->icon, ctx, glyph_frame);
    }

    text_set_highlighted(&item->icon, 0);
    return 1;
}

#include <dlfcn.h>

/* Space ids are switched through SkyLight. The framework is dlopen'd
   RTLD_LOCAL (see private_shim.c), so dlsym(RTLD_DEFAULT, ...) can never see
   its symbols - the lookup has to go through the same handle the shim uses.

   Two incompatible spellings exist. Up to macOS 14 the call was
   SLSSwitchToSpace(connection, space_id). macOS 15 dropped it in favour of
   SLSManagedDisplaySetCurrentSpace, which additionally takes the target
   display's "Display Identifier" UUID string; passing a space id there
   dereferences it as an object and takes the process down. The UUID-first
   shape is confirmed by the framework's own bridged operation, whose
   -initWithDisplayIdentifier:spaceID: takes an object then a uint64. */
static CGError bar_item_space_switch(uint64_t sid) {
    typedef CGError (*legacy_fn)(uint32_t, uint64_t);
    typedef CGError (*uuid_fn)(uint32_t, CFStringRef, uint64_t);
    static legacy_fn legacy = NULL;
    static uuid_fn by_uuid = NULL;
    static int looked_up = 0;
    static int warned = 0;

    if (!looked_up) {
        looked_up = 1;
        legacy = (legacy_fn)private_symbol("SLSSwitchToSpace");
        if (!legacy) legacy = (legacy_fn)private_symbol("CGSSwitchToSpace");
        by_uuid = (uuid_fn)private_symbol("SLSManagedDisplaySetCurrentSpace");
    }

    if (legacy) return legacy(g_connection, sid);

    if (by_uuid) {
        CFStringRef uuid = display_space_display_identifier(sid);
        if (!uuid) {
            const char *msg = "omabar: no managed display reports the target "
                              "space; cannot switch space";
            fprintf(stderr, "%s\n", msg);
            syslog(LOG_ERR, "%s", msg);
            return -1;
        }
        CGError err = by_uuid(g_connection, uuid, sid);
        CFRelease(uuid);
        return err;
    }

    if (!warned) {
        warned = 1;
        const char *msg = "omabar: no space-switch symbol found in SkyLight; "
                          "space chips will not change spaces on this macOS";
        fprintf(stderr, "%s\n", msg);
        syslog(LOG_ERR, "%s", msg);
    }
    return -1;
}

void bar_item_space_clicked(struct bar_item *item, CGPoint point) {
    if (!item || item->type != BAR_COMPONENT_SPACE) return;

    /* resolve which chip was hit; `point` is in the item's local space */
    uint64_t target = 0;
    if (item->space_id > 0) {
        target = item->space_id;
    } else {
        int count = item->space_count > 0 ? item->space_count : 1;
        float chip_w = bar_item_space_chip_width(item)
                       + (float)item->space_gap;
        int index = (int)floorf(point.x / (chip_w > 0.0f ? chip_w : 1.0f));
        if (index < 0) index = 0;
        if (index >= count) index = count - 1;
        target = item->space_ids[index];
    }

    if (target < 1) return;
    bar_item_space_switch((uint64_t)target);

    if (item->click_script)
        bar_item_update(item, "mouse.clicked", NULL);
}
