#include "group.h"
#include "misc/helpers.h"
#include <stdlib.h>

void group_create(struct group *group, struct bar_item *parent) {
    memset(group, 0, sizeof(*group));
    group->bracket = parent;
}

void group_destroy(struct group *group) {
    if (!group) return;
    free(group->items);
    group->items = NULL;
    group->item_count = 0;
}

void group_add_item(struct group *group, struct bar_item *item) {
    if (!group || !item) return;
    buf_push(group->items, item);
    item->group = group;
}

void group_remove_item(struct group *group, struct bar_item *item) {
    if (!group || !item) return;
    for (int i = 0; i < group->item_count; i++) {
        if (group->items[i] == item) {
            memmove(&group->items[i], &group->items[i + 1],
                    (group->item_count - i - 1) * sizeof(struct bar_item *));
            group->item_count--;
            item->group = NULL;
            return;
        }
    }
}