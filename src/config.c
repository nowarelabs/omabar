#include "config.h"
#include "bar_manager.h"
#include "bar_item.h"
#include "color.h"
#include "font.h"
#include "misc/defines.h"
#include "misc/helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#undef BAR_COMPONENT_SPACE
#undef BAR_COMPONENT_ALIAS
#undef BAR_COMPONENT_GROUP
#undef BAR_COMPONENT_GRAPH
#undef BAR_COMPONENT_SLIDER

extern struct bar_manager g_bar_manager;

/* ──────────────────────────────────────────────────────────────────────────
   Minimal JSON AST Parser
   ────────────────────────────────────────────────────────────────────────── */

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type_t;

typedef struct json_val json_val_t;

typedef struct json_pair {
    char *key;
    json_val_t *val;
} json_pair_t;

struct json_val {
    json_type_t type;
    union {
        bool bool_val;
        double num_val;
        char *str_val;
        struct {
            json_val_t **items;
            size_t count;
        } array;
        struct {
            json_pair_t *pairs;
            size_t count;
        } object;
    };
};

static void json_val_free(json_val_t *v) {
    if (!v) return;
    switch (v->type) {
        case JSON_STRING:
            free(v->str_val);
            break;
        case JSON_ARRAY:
            for (size_t i = 0; i < v->array.count; i++) {
                json_val_free(v->array.items[i]);
            }
            free(v->array.items);
            break;
        case JSON_OBJECT:
            for (size_t i = 0; i < v->object.count; i++) {
                free(v->object.pairs[i].key);
                json_val_free(v->object.pairs[i].val);
            }
            free(v->object.pairs);
            break;
        default:
            break;
    }
    free(v);
}

static void json_skip_ws(const char **p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

static size_t utf8_encode_cp(uint32_t cp, char *buf, size_t len) {
    if (cp < 0x80) {
        buf[len++] = (char)cp;
    } else if (cp < 0x800) {
        buf[len++] = (char)(0xC0 | (cp >> 6));
        buf[len++] = (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        buf[len++] = (char)(0xE0 | (cp >> 12));
        buf[len++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[len++] = (char)(0x80 | (cp & 0x3F));
    } else {
        buf[len++] = (char)(0xF0 | (cp >> 18));
        buf[len++] = (char)(0x80 | ((cp >> 12) & 0x3F));
        buf[len++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[len++] = (char)(0x80 | (cp & 0x3F));
    }
    return len;
}

static int hex_digit_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char *json_parse_raw_string(const char **p) {
    if (**p != '"') return NULL;
    (*p)++;

    size_t len = 0, cap = 32;
    char *out = malloc(cap);
    if (!out) return NULL;

    while (**p && **p != '"') {
        uint32_t c = (unsigned char)**p;
        (*p)++;
        if (c == '\\') {
            switch (**p) {
                case '"':  c = '"';  (*p)++; break;
                case '\\': c = '\\'; (*p)++; break;
                case '/':  c = '/';  (*p)++; break;
                case 'b':  c = '\b'; (*p)++; break;
                case 'f':  c = '\f'; (*p)++; break;
                case 'n':  c = '\n'; (*p)++; break;
                case 'r':  c = '\r'; (*p)++; break;
                case 't':  c = '\t'; (*p)++; break;
                case 'u': {
                    (*p)++;
                    uint32_t cp = 0;
                    int valid = 1;
                    for (int i = 0; i < 4; i++) {
                        int h = hex_digit_val(**p);
                        if (h < 0) { valid = 0; break; }
                        cp = (cp << 4) | (uint32_t)h;
                        (*p)++;
                    }
                    if (!valid) { c = '?'; break; }
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (**p == '\\' && (*p)[1] == 'u') {
                            const char *q = *p + 2;
                            uint32_t lo = 0;
                            for (int i = 0; i < 4; i++) {
                                int h = hex_digit_val(*q);
                                if (h < 0) { lo = 0; break; }
                                lo = (lo << 4) | (uint32_t)h;
                                q++;
                            }
                            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                                *p = q;
                            }
                        }
                    }
                    if (len + 4 >= cap) {
                        cap *= 2;
                        char *new_out = realloc(out, cap);
                        if (!new_out) { free(out); return NULL; }
                        out = new_out;
                    }
                    len = utf8_encode_cp(cp, out, len);
                    continue;
                }
                default:
                    c = (unsigned char)**p;
                    if (**p) (*p)++;
                    break;
            }
        }
        if (len + 1 >= cap) {
            cap *= 2;
            char *new_out = realloc(out, cap);
            if (!new_out) { free(out); return NULL; }
            out = new_out;
        }
        out[len++] = (char)c;
    }

    if (**p != '"') { free(out); return NULL; }
    (*p)++;

    out[len] = '\0';
    return out;
}

static json_val_t *json_parse_val(const char **p);

static json_val_t *json_parse_array(const char **p) {
    if (**p != '[') return NULL;
    (*p)++;

    json_val_t *v = calloc(1, sizeof(*v));
    v->type = JSON_ARRAY;

    json_skip_ws(p);
    if (**p == ']') {
        (*p)++;
        return v;
    }

    while (**p) {
        json_val_t *elem = json_parse_val(p);
        if (!elem) { json_val_free(v); return NULL; }

        v->array.items = realloc(v->array.items, sizeof(json_val_t *) * (v->array.count + 1));
        v->array.items[v->array.count++] = elem;

        json_skip_ws(p);
        if (**p == ']') {
            (*p)++;
            return v;
        }
        if (**p == ',') {
            (*p)++;
            json_skip_ws(p);
        } else {
            json_val_free(v);
            return NULL;
        }
    }

    json_val_free(v);
    return NULL;
}

static json_val_t *json_parse_object(const char **p) {
    if (**p != '{') return NULL;
    (*p)++;

    json_val_t *v = calloc(1, sizeof(*v));
    v->type = JSON_OBJECT;

    json_skip_ws(p);
    if (**p == '}') {
        (*p)++;
        return v;
    }

    while (**p) {
        json_skip_ws(p);
        if (**p != '"') { json_val_free(v); return NULL; }
        char *key = json_parse_raw_string(p);
        if (!key) { json_val_free(v); return NULL; }

        json_skip_ws(p);
        if (**p != ':') { free(key); json_val_free(v); return NULL; }
        (*p)++;

        json_val_t *val = json_parse_val(p);
        if (!val) { free(key); json_val_free(v); return NULL; }

        v->object.pairs = realloc(v->object.pairs, sizeof(json_pair_t) * (v->object.count + 1));
        v->object.pairs[v->object.count].key = key;
        v->object.pairs[v->object.count].val = val;
        v->object.count++;

        json_skip_ws(p);
        if (**p == '}') {
            (*p)++;
            return v;
        }
        if (**p == ',') {
            (*p)++;
            json_skip_ws(p);
        } else {
            json_val_free(v);
            return NULL;
        }
    }

    json_val_free(v);
    return NULL;
}

static json_val_t *json_parse_val(const char **p) {
    json_skip_ws(p);
    if (!**p) return NULL;

    if (**p == '{') return json_parse_object(p);
    if (**p == '[') return json_parse_array(p);
    if (**p == '"') {
        char *s = json_parse_raw_string(p);
        if (!s) return NULL;
        json_val_t *v = calloc(1, sizeof(*v));
        v->type = JSON_STRING;
        v->str_val = s;
        return v;
    }
    if (strncmp(*p, "true", 4) == 0) {
        *p += 4;
        json_val_t *v = calloc(1, sizeof(*v));
        v->type = JSON_BOOL;
        v->bool_val = true;
        return v;
    }
    if (strncmp(*p, "false", 5) == 0) {
        *p += 5;
        json_val_t *v = calloc(1, sizeof(*v));
        v->type = JSON_BOOL;
        v->bool_val = false;
        return v;
    }
    if (strncmp(*p, "null", 4) == 0) {
        *p += 4;
        json_val_t *v = calloc(1, sizeof(*v));
        v->type = JSON_NULL;
        return v;
    }
    if (**p == '-' || isdigit((unsigned char)**p)) {
        char *end = NULL;
        double n = strtod(*p, &end);
        if (end == *p) return NULL;
        *p = end;
        json_val_t *v = calloc(1, sizeof(*v));
        v->type = JSON_NUMBER;
        v->num_val = n;
        return v;
    }

    return NULL;
}

/* Helper functions to retrieve values from JSON AST */

static json_val_t *json_get_field(const json_val_t *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT || !key) return NULL;
    for (size_t i = 0; i < obj->object.count; i++) {
        if (strcmp(obj->object.pairs[i].key, key) == 0) {
            return obj->object.pairs[i].val;
        }
    }
    return NULL;
}

static const char *json_get_str(const json_val_t *obj, const char *key, const char *def) {
    json_val_t *f = json_get_field(obj, key);
    if (f && f->type == JSON_STRING && f->str_val) return f->str_val;
    return def;
}

static double json_get_num(const json_val_t *obj, const char *key, double def) {
    json_val_t *f = json_get_field(obj, key);
    if (f && f->type == JSON_NUMBER) return f->num_val;
    return def;
}

static bool json_get_bool(const json_val_t *obj, const char *key, bool def) {
    json_val_t *f = json_get_field(obj, key);
    if (f && f->type == JSON_BOOL) return f->bool_val;
    return def;
}

/* ──────────────────────────────────────────────────────────────────────────
   OMABC Configuration Loader logic
   ────────────────────────────────────────────────────────────────────────── */

static uint64_t map_event_string(const char *name) {
    if (!name) return 0;
    if (strcmp(name, "space_changed") == 0) return UPDATE_SPACE_CHANGED;
    if (strcmp(name, "display_changed") == 0)
        return UPDATE_DISPLAY_ADDED | UPDATE_DISPLAY_REMOVED | UPDATE_DISPLAY_MOVED;
    if (strcmp(name, "display_added") == 0) return UPDATE_DISPLAY_ADDED;
    if (strcmp(name, "display_removed") == 0) return UPDATE_DISPLAY_REMOVED;
    if (strcmp(name, "display_moved") == 0) return UPDATE_DISPLAY_MOVED;
    if (strcmp(name, "power_changed") == 0) return UPDATE_POWER_CHANGED;
    if (strcmp(name, "tick") == 0 || strcmp(name, "routine") == 0) return UPDATE_ROUTINE;
    if (strcmp(name, "scroll.tick") == 0 || strcmp(name, "scroll_tick") == 0) return UPDATE_SCROLL_TICK;
    if (strcmp(name, "media_changed") == 0) return UPDATE_MEDIA_CHANGED;
    if (strcmp(name, "front_app_changed") == 0 || strcmp(name, "front_app_switched") == 0)
        return UPDATE_FRONT_APP_SWITCHED;
    if (strcmp(name, "volume_changed") == 0 || strcmp(name, "mute_changed") == 0) return UPDATE_VOLUME_CHANGED;
    if (strcmp(name, "wifi_changed") == 0) return UPDATE_WIFI_CHANGED;
    if (strcmp(name, "mouse_clicked") == 0 || strcmp(name, "mouse.clicked") == 0) return UPDATE_MOUSE_CLICKED;
    if (strcmp(name, "mouse_entered") == 0 || strcmp(name, "mouse.entered") == 0) return UPDATE_MOUSE_ENTERED;
    if (strcmp(name, "mouse_exited") == 0 || strcmp(name, "mouse.exited") == 0) return UPDATE_MOUSE_EXITED;
    if (strcmp(name, "mouse_scrolled") == 0 || strcmp(name, "mouse.scrolled") == 0) return UPDATE_MOUSE_SCROLLED;
    if (strcmp(name, "mouse_dragged") == 0 || strcmp(name, "mouse.dragged") == 0) return UPDATE_MOUSE_DRAGGED;
    if (strcmp(name, "window_focused") == 0) return UPDATE_WINDOW_FOCUSED;
    if (strcmp(name, "brightness_changed") == 0) return UPDATE_BRIGHTNESS_CHANGED;
    if (strcmp(name, "daemon_message") == 0) return UPDATE_DAEMON_MESSAGE;
    if (strcmp(name, "dnd") == 0) return UPDATE_DND;
    return 0;
}

static struct font *parse_font_spec(const char *font_str) {
    if (!font_str || !*font_str) return NULL;
    struct font *f = calloc(1, sizeof(*f));
    font_init(f);
    font_set(f, strdup(font_str), true);
    font_create_ctfont(f);
    return f;
}

static enum bar_item_type parse_item_type(const char *tstr) {
    if (!tstr) return BAR_ITEM;
    if (strcmp(tstr, "space") == 0) return BAR_COMPONENT_SPACE;
    if (strcmp(tstr, "alias") == 0) return BAR_COMPONENT_ALIAS;
    if (strcmp(tstr, "group") == 0) return BAR_COMPONENT_GROUP;
    if (strcmp(tstr, "graph") == 0) return BAR_COMPONENT_GRAPH;
    if (strcmp(tstr, "slider") == 0) return BAR_COMPONENT_SLIDER;
    return BAR_ITEM;
}

static enum bar_item_position parse_item_position(const char *pstr, enum bar_item_position default_pos) {
    if (!pstr) return default_pos;
    if (strcmp(pstr, "l") == 0 || strcmp(pstr, "left") == 0) return POSITION_LEFT;
    if (strcmp(pstr, "r") == 0 || strcmp(pstr, "right") == 0) return POSITION_RIGHT;
    if (strcmp(pstr, "c") == 0 || strcmp(pstr, "center") == 0) return POSITION_CENTER;
    if (strcmp(pstr, "q") == 0 || strcmp(pstr, "center_left") == 0) return POSITION_CENTER_LEFT;
    if (strcmp(pstr, "e") == 0 || strcmp(pstr, "center_right") == 0) return POSITION_CENTER_RIGHT;
    if (strcmp(pstr, "p") == 0 || strcmp(pstr, "popup") == 0) return POSITION_POPUP;
    return default_pos;
}

static void apply_background_json(struct background *bg, const json_val_t *bg_obj) {
    if (!bg || !bg_obj || bg_obj->type != JSON_OBJECT) return;

    const char *col_str = json_get_str(bg_obj, "color", NULL);
    if (col_str) bg->color = color_from_hex_string(col_str);

    const char *bcol_str = json_get_str(bg_obj, "border_color", NULL);
    if (bcol_str) bg->border_color = color_from_hex_string(bcol_str);

    bg->corner_radius = (int)json_get_num(bg_obj, "corner_radius", bg->corner_radius);
    bg->border_width = (int)json_get_num(bg_obj, "border_width", bg->border_width);
}

static void parse_item_object(const json_val_t *item_obj,
                              enum bar_item_position default_pos,
                              const json_val_t *defaults_obj) {
    if (!item_obj || item_obj->type != JSON_OBJECT) return;

    const char *name = json_get_str(item_obj, "name", NULL);
    if (!name) return;

    struct bar_item *item = calloc(1, sizeof(*item));
    bar_item_init(item);
    bar_item_set_name(item, name);

    /* Apply defaults first if provided */
    if (defaults_obj && defaults_obj->type == JSON_OBJECT) {
        json_val_t *def_bg = json_get_field(defaults_obj, "background");
        apply_background_json(&item->background, def_bg);

        json_val_t *def_icon = json_get_field(defaults_obj, "icon");
        if (def_icon && def_icon->type == JSON_OBJECT) {
            const char *f = json_get_str(def_icon, "font", NULL);
            if (f) text_set_font(&item->icon, parse_font_spec(f));
            const char *c = json_get_str(def_icon, "color", NULL);
            if (c) text_set_color(&item->icon, color_from_hex_string(c));
            const char *hc = json_get_str(def_icon, "highlight_color", NULL);
            if (hc) text_set_highlight_color(&item->icon, color_from_hex_string(hc));
        }

        json_val_t *def_label = json_get_field(defaults_obj, "label");
        if (def_label && def_label->type == JSON_OBJECT) {
            const char *f = json_get_str(def_label, "font", NULL);
            if (f) text_set_font(&item->label, parse_font_spec(f));
            const char *c = json_get_str(def_label, "color", NULL);
            if (c) text_set_color(&item->label, color_from_hex_string(c));
        }

        item->padding_left = (int)json_get_num(defaults_obj, "padding_left", item->padding_left);
        item->padding_right = (int)json_get_num(defaults_obj, "padding_right", item->padding_right);
    }

    /* Item-specific overrides */
    const char *type_str = json_get_str(item_obj, "type", NULL);
    enum bar_item_type itype = parse_item_type(type_str);
    bar_item_set_type(item, itype);

    const char *pos_str = json_get_str(item_obj, "position", NULL);
    item->position = parse_item_position(pos_str, default_pos);

    item->click_enabled = json_get_bool(item_obj, "click_enabled", item->click_enabled) ? 1 : 0;
    item->associated_display = (uint32_t)json_get_num(item_obj, "associated_display", -1.0);
    item->associated_space = (uint32_t)json_get_num(item_obj, "associated_space", -1.0);
    item->update_interval = (int)json_get_num(item_obj, "update_interval", 0);
    item->y_offset = (int)json_get_num(item_obj, "y_offset", item->y_offset);
    item->padding_left = (int)json_get_num(item_obj, "padding_left", item->padding_left);
    item->padding_right = (int)json_get_num(item_obj, "padding_right", item->padding_right);
    item->label_x_offset = (int)json_get_num(item_obj, "label_x_offset", item->label_x_offset);
    item->icon_x_offset = (int)json_get_num(item_obj, "icon_x_offset", item->icon_x_offset);

    const char *script = json_get_str(item_obj, "script", NULL);
    if (script) bar_item_set_script(item, script);

    const char *click_script = json_get_str(item_obj, "click_script", NULL);
    if (click_script) bar_item_set_click_script(item, click_script);

    /* Background overrides */
    json_val_t *bg = json_get_field(item_obj, "background");
    apply_background_json(&item->background, bg);

    /* Icon overrides */
    json_val_t *icon_obj = json_get_field(item_obj, "icon");
    if (icon_obj && icon_obj->type == JSON_OBJECT) {
        const char *f = json_get_str(icon_obj, "font", NULL);
        if (f) text_set_font(&item->icon, parse_font_spec(f));
        const char *c = json_get_str(icon_obj, "color", NULL);
        if (c) text_set_color(&item->icon, color_from_hex_string(c));
        const char *hc = json_get_str(icon_obj, "highlight_color", NULL);
        if (hc) text_set_highlight_color(&item->icon, color_from_hex_string(hc));
        const char *s = json_get_str(icon_obj, "string", NULL);
        if (s) text_set_string(&item->icon, s);
    }
    const char *icon_hc = json_get_str(item_obj, "icon_highlight_color", NULL);
    if (icon_hc) text_set_highlight_color(&item->icon, color_from_hex_string(icon_hc));

    /* Label overrides */
    json_val_t *label_obj = json_get_field(item_obj, "label");
    if (label_obj && label_obj->type == JSON_OBJECT) {
        const char *f = json_get_str(label_obj, "font", NULL);
        if (f) text_set_font(&item->label, parse_font_spec(f));
        const char *c = json_get_str(label_obj, "color", NULL);
        if (c) text_set_color(&item->label, color_from_hex_string(c));
        const char *s = json_get_str(label_obj, "string", NULL);
        if (s) text_set_string(&item->label, s);
    }

    /* Icon strip for space components */
    json_val_t *strip_obj = json_get_field(item_obj, "icon_strip");
    if (strip_obj && strip_obj->type == JSON_ARRAY && strip_obj->array.count > 0) {
        char **strip = malloc(sizeof(char *) * strip_obj->array.count);
        int count = 0;
        for (size_t i = 0; i < strip_obj->array.count; i++) {
            json_val_t *elem = strip_obj->array.items[i];
            if (elem && elem->type == JSON_STRING && elem->str_val) {
                strip[count++] = elem->str_val;
            }
        }
        bar_item_set_icon_strip(item, strip, count);
        free(strip);
    }

    /* Update mask */
    json_val_t *umask_obj = json_get_field(item_obj, "update_mask");
    if (umask_obj && umask_obj->type == JSON_ARRAY) {
        for (size_t i = 0; i < umask_obj->array.count; i++) {
            json_val_t *elem = umask_obj->array.items[i];
            if (elem && elem->type == JSON_STRING && elem->str_val) {
                item->update_mask |= map_event_string(elem->str_val);
            }
        }
    }

    bar_manager_add_item(&g_bar_manager, item);
}

int config_load(const char *path, char *out_lock_file, size_t lock_file_size) {
    if (!path || !*path) {
        fprintf(stderr, "omabar: no configuration file path provided\n");
        return -1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "omabar: failed to open config file '%s'\n", path);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 16) {
        fprintf(stderr, "omabar: config file '%s' is too small (%ld bytes, expected at least 16)\n",
                path, size);
        fclose(f);
        return -1;
    }

    char *buf = malloc((size_t)size + 1);
    if (!buf) {
        fprintf(stderr, "omabar: memory allocation failed loading config\n");
        fclose(f);
        return -1;
    }

    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "omabar: failed to read complete config file '%s'\n", path);
        fclose(f);
        free(buf);
        return -1;
    }
    fclose(f);
    buf[size] = '\0';

    /* Verify OMABC magic header (5 bytes) */
    if (strncmp(buf, "OMABC", 5) != 0) {
        fprintf(stderr, "omabar: invalid OMABC config file header magic in '%s'\n", path);
        free(buf);
        return -1;
    }

    /* JSON payload starts at byte offset 16 */
    const char *json_ptr = buf + 16;
    const char *p = json_ptr;
    json_val_t *root = json_parse_val(&p);
    if (!root || root->type != JSON_OBJECT) {
        fprintf(stderr, "omabar: failed to parse JSON payload in config file '%s'\n", path);
        json_val_free(root);
        free(buf);
        return -1;
    }

    /* 1. Read daemon settings */
    json_val_t *daemon_obj = json_get_field(root, "daemon");
    if (daemon_obj && daemon_obj->type == JSON_OBJECT) {
        const char *lf = json_get_str(daemon_obj, "lock_file", NULL);
        if (lf && *lf && out_lock_file && lock_file_size > 0) {
            snprintf(out_lock_file, lock_file_size, "%s", lf);
        }
    }

    /* 2. Read bar settings */
    json_val_t *bar_obj = json_get_field(root, "bar");
    if (bar_obj && bar_obj->type == JSON_OBJECT) {
        const char *pos = json_get_str(bar_obj, "position", "top");
        int pos_val = 0; /* top */
        if (strcmp(pos, "bottom") == 0) pos_val = 1;
        else if (strcmp(pos, "left") == 0) pos_val = 2;
        else if (strcmp(pos, "right") == 0) pos_val = 3;
        bar_manager_set_position(&g_bar_manager, pos_val);

        int height = (int)json_get_num(bar_obj, "height", 38);
        if (height > 0) bar_manager_set_height(&g_bar_manager, height);

        int width = (int)json_get_num(bar_obj, "width", 0);
        if (width >= 0) bar_manager_set_width(&g_bar_manager, width);

        int margin = (int)json_get_num(bar_obj, "margin", 0);
        bar_manager_set_margin(&g_bar_manager, margin);

        int blur = (int)json_get_num(bar_obj, "blur_radius", 0);
        bar_manager_set_blur_radius(&g_bar_manager, blur);

        int shadow = json_get_bool(bar_obj, "shadow", false) ? 1 : 0;
        bar_manager_set_shadow(&g_bar_manager, shadow);

        int sticky = json_get_bool(bar_obj, "sticky", true) ? 1 : 0;
        bar_manager_set_sticky(&g_bar_manager, sticky);

        int topmost = json_get_bool(bar_obj, "topmost", true) ? 1 : 0;
        bar_manager_set_topmost(&g_bar_manager, topmost);

        int y_offset = (int)json_get_num(bar_obj, "y_offset", 0);
        bar_manager_set_y_offset(&g_bar_manager, y_offset);

        double alpha = json_get_num(bar_obj, "alpha", 1.0);
        bar_manager_set_alpha(&g_bar_manager, alpha);

        const char *bar_col = json_get_str(bar_obj, "color", NULL);
        if (bar_col) {
            struct color c = color_from_hex_string(bar_col);
            if (color_is_valid(c)) {
                bar_manager_set_background_color(&g_bar_manager, c);
            }
        }
    }

    /* 3. Read defaults & items */
    json_val_t *defaults_obj = json_get_field(root, "defaults");
    json_val_t *items_obj = json_get_field(root, "items");

    if (items_obj && items_obj->type == JSON_OBJECT) {
        struct {
            const char *key;
            enum bar_item_position pos;
        } sections[] = {
            { "left", POSITION_LEFT },
            { "center_left", POSITION_CENTER_LEFT },
            { "center", POSITION_CENTER },
            { "center_right", POSITION_CENTER_RIGHT },
            { "right", POSITION_RIGHT }
        };

        for (size_t s = 0; s < sizeof(sections) / sizeof(sections[0]); s++) {
            json_val_t *sec_arr = json_get_field(items_obj, sections[s].key);
            if (sec_arr && sec_arr->type == JSON_ARRAY) {
                for (size_t i = 0; i < sec_arr->array.count; i++) {
                    parse_item_object(sec_arr->array.items[i], sections[s].pos, defaults_obj);
                }
            }
        }
    }

    /* 4. Read events.items mappings if present */
    json_val_t *events_obj = json_get_field(root, "events");
    if (events_obj && events_obj->type == JSON_OBJECT) {
        json_val_t *ev_items = json_get_field(events_obj, "items");
        if (ev_items && ev_items->type == JSON_ARRAY) {
            for (size_t i = 0; i < ev_items->array.count; i++) {
                json_val_t *entry = ev_items->array.items[i];
                if (entry && entry->type == JSON_OBJECT) {
                    const char *ename = json_get_str(entry, "event", NULL);
                    const char *iname = json_get_str(entry, "item", NULL);
                    if (ename && iname) {
                        uint64_t mask = map_event_string(ename);
                        if (mask > 0) {
                            for (int k = 0; k < g_bar_manager.bar_item_count; k++) {
                                struct bar_item *it = g_bar_manager.bar_items[k];
                                if (it && it->name && strcmp(it->name, iname) == 0) {
                                    it->update_mask |= mask;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    json_val_free(root);
    free(buf);
    return 0;
}
