#include "group.h"
#include "window.h"
#include <stdlib.h>
#include <string.h>

void group_init(struct group *group) {
    if (!group) return;
    group->items = NULL;
    group->item_count = 0;
    group->bracket = NULL;
    group->bracket_recursive = false;
    group->bounds = CGRectNull;
}

void group_destroy(struct group *group) {
    if (!group) return;
    if (group->items) {
        for (int i = 0; i < group->item_count; i++) {
            if (group->items[i])
                group->items[i]->group = NULL;
        }
        free(group->items);
        group->items = NULL;
    }
    group->item_count = 0;
}

bool group_is_item_member(struct group *group, struct bar_item *item) {
    if (!group || !item) return false;
    for (int i = 0; i < group->item_count; i++) {
        if (group->items[i] == item) return true;
    }
    return false;
}

void group_add_item(struct group *group, struct bar_item *item) {
    if (!group || !item) return;
    if (group_is_item_member(group, item)) return;

    if (item->group && item->group != group) {
        for (int i = 0; i < item->group->item_count; i++) {
            if (item->group->items[i] == item) {
                item->group->item_count--;
                if (item->group->item_count == 0) {
                    free(item->group->items);
                    item->group->items = NULL;
                } else {
                    memmove(&item->group->items[i],
                            &item->group->items[i + 1],
                            (item->group->item_count - i) * sizeof(void *));
                    item->group->items = realloc(
                        item->group->items,
                        item->group->item_count * sizeof(void *));
                }
                break;
            }
        }
    }

    group->item_count++;
    group->items = realloc(group->items,
                           group->item_count * sizeof(void *));
    group->items[group->item_count - 1] = item;
    item->group = group;
}

void group_remove_item(struct group *group, struct bar_item *item) {
    if (!group || !item) return;
    for (int i = 0; i < group->item_count; i++) {
        if (group->items[i] == item) {
            group->item_count--;
            if (group->item_count == 0) {
                free(group->items);
                group->items = NULL;
            } else {
                memmove(&group->items[i], &group->items[i + 1],
                        (group->item_count - i) * sizeof(void *));
                group->items = realloc(group->items,
                                       group->item_count * sizeof(void *));
            }
            item->group = NULL;
            return;
        }
    }
}

static CGRect get_first_window_rect(struct bar_item *item) {
    if (!item || item->window_count == 0 || !item->windows)
        return CGRectNull;
    struct window *w = item->windows[0];
    if (!w) return CGRectNull;
    return (CGRect){ w->origin, w->frame.size };
}

void group_calculate_bounds(struct group *group) {
    if (!group || group->item_count == 0) {
        if (group) group->bounds = CGRectNull;
        return;
    }

    CGFloat min_x = CGFLOAT_MAX;
    CGFloat max_right = -CGFLOAT_MAX;
    CGFloat height = 0;

    for (int i = 0; i < group->item_count; i++) {
        CGRect r = get_first_window_rect(group->items[i]);
        if (CGRectIsNull(r)) continue;
        if (r.origin.x < min_x) min_x = r.origin.x;
        CGFloat right = r.origin.x + r.size.width;
        if (right > max_right) max_right = right;
        if (r.size.height > height) height = r.size.height;
    }

    if (min_x == CGFLOAT_MAX) {
        group->bounds = CGRectNull;
        return;
    }

    group->bounds = (CGRect){{ min_x, 0 },
                             { max_right - min_x, height }};
}