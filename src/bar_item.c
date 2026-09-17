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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct bar_manager g_bar_manager;

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
    text_init(&item->icon);
    text_init(&item->label);
    background_init(&item->background);
    item->popup = calloc(1, sizeof(*item->popup));
    if (item->popup) popup_init(item->popup, item);
}

void bar_item_destroy(struct bar_item *item) {
    if (!item) return;
    free(item->name);
    text_destroy(&item->icon);
    text_destroy(&item->label);
    background_destroy(&item->background);
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

    text_clone(&dst->icon, &src->icon);
    text_clone(&dst->label, &src->label);
    background_clone(&dst->background, &src->background);

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

float bar_item_get_content_length(struct bar_item *item) {
    if (!item) return 0;
    float length = item->icon.line.width;
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
        bar_item_space_clicked(item);
        return;
    }

    /* a host item with a popup toggles it on click */
    if (item->popup && popup_has_items(item->popup)
        && item->popup->drawing && button == 1) {
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
    float icon_w = item->icon.line.width;
    float label_w = item->label.line.width;

    float comp_w = 0;
    if (item->graph) comp_w = (float)item->graph->width;
    else if (item->alias) {
        comp_w = item->alias->image
                 ? (float)CGImageGetWidth(item->alias->image)
                 : (float)item->alias->width;
    }
    else if (item->slider) comp_w = (float)item->slider->width;

    item->icon.x_offset = item->padding_left + item->icon_x_offset;
    item->label.x_offset = item->padding_left + (int)icon_w + (int)comp_w
                           + item->label_x_offset;

    float content_w = icon_w + comp_w + label_w;
    float total_w = item->padding_left + content_w + item->padding_right;
    float total_h = 30.0f;

    return CGRectMake(0, (float)item->y_offset, total_w, total_h);
}

void bar_item_draw(struct bar_item *item, struct bar *bar, CGContextRef ctx) {
    (void)bar;
    if (!item || !ctx || item->hidden) return;

    /* space component: chip highlight + icon_strip glyph */
    if (item->type == BAR_COMPONENT_SPACE) {
        float sc = bar_item_get_content_length(item);
        float sw = item->padding_left + sc + item->padding_right;
        float sh = 30.0f;
        CGRect sframe = CGRectMake(0, (float)item->y_offset, sw, sh);
        bar_item_space_draw(item, bar, ctx, sframe);
        return;
    }

    float content = bar_item_get_content_length(item);
    float total_w = item->padding_left + content + item->padding_right;
    float total_h = 30.0f;
    CGRect frame = CGRectMake(0, (float)item->y_offset, total_w, total_h);

    /* background layer */
    if (item->background.type > 0 || item->background.color.a > 0
        || item->background.has_shadow) {
        background_draw(&item->background, ctx, frame);
    }

    /* icon */
    float cursor = item->padding_left + item->icon_x_offset;
    float icon_h = item->icon.line.ascent + item->icon.line.descent;
    float y = (total_h - icon_h) / 2.0f;

    CGRect icon_frame = CGRectMake(cursor, y + (float)item->y_offset,
                                   item->icon.line.width, icon_h);
    text_draw(&item->icon, ctx, icon_frame);
    cursor += item->icon.line.width;

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
static const char *bar_item_space_icon(struct bar_item *item) {
    if (!item->icon_strip || item->icon_strip_count <= 0) return NULL;
    if (item->space_id >= 1 && item->space_id <= (uint64_t)item->icon_strip_count)
        return item->icon_strip[item->space_id - 1];
    return NULL;
}

int bar_item_space_draw(struct bar_item *item, struct bar *bar, CGContextRef ctx, CGRect frame) {
    (void)bar;
    if (!item || !ctx || item->hidden) return 0;
    if (item->type != BAR_COMPONENT_SPACE) return 0;

    /* selected highlight */
    if (item->selected) {
        struct background *bg = &item->background;
        if (bg->color.a <= 0.0f) {
            struct color sel = { 1.0f, 1.0f, 1.0f, 0.35f, 1 };
            struct background tmp = *bg;
            tmp.color = sel;
            background_draw(&tmp, ctx, frame);
        } else {
            background_draw(bg, ctx, frame);
        }
    } else if (item->background.type > 0 || item->background.color.a > 0
               || item->background.has_shadow) {
        background_draw(&item->background, ctx, frame);
    }

    /* glyph: icon_strip entry for this space, or the space number */
    const char *glyph = bar_item_space_icon(item);
    char numbuf[16];
    if (!glyph) {
        snprintf(numbuf, sizeof(numbuf), "%llu",
                 (unsigned long long)(item->space_id ? item->space_id : 1));
        glyph = numbuf;
    }
    text_set_string(&item->icon, glyph);

    /* draw glyph centered in the chip */
    float glyph_w = item->icon.line.width;
    float glyph_h = item->icon.line.ascent + item->icon.line.descent;
    float x = frame.origin.x + (frame.size.width - glyph_w) / 2.0f;
    float y = frame.origin.y + (frame.size.height - glyph_h) / 2.0f;

    CGRect glyph_frame = CGRectMake(x, y, glyph_w, glyph_h);
    text_draw(&item->icon, ctx, glyph_frame);
    return 1;
}

#include <dlfcn.h>

static CGError bar_item_space_switch(uint64_t sid) {
    typedef CGError (*switch_fn)(uint32_t, uint64_t);
    static switch_fn sfn = NULL;
    static int looked_up = 0;
    if (!looked_up) {
        looked_up = 1;
        sfn = (switch_fn)dlsym(RTLD_DEFAULT, "SLSSwitchToSpace");
        if (!sfn) sfn = (switch_fn)dlsym(RTLD_DEFAULT, "CGSSwitchToSpace");
    }
    if (!sfn) return -1;
    return sfn(g_connection, sid);
}

void bar_item_space_clicked(struct bar_item *item) {
    if (!item || item->type != BAR_COMPONENT_SPACE) return;
    if (item->space_id < 1) return;

    bar_item_space_switch((uint64_t)item->space_id);

    if (item->click_script)
        bar_item_update(item, "mouse.clicked", NULL);
}
