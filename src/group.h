#ifndef OMABAR_GROUP_H
#define OMABAR_GROUP_H

#include "bar_item.h"
#include <stdbool.h>

struct group {
    struct bar_item **items;
    int item_count;
    struct bar_item *bracket;
    bool bracket_recursive;
    CGRect bounds;
};

void group_init(struct group *group);
void group_destroy(struct group *group);
bool group_is_item_member(struct group *group, struct bar_item *item);
void group_add_item(struct group *group, struct bar_item *item);
void group_remove_item(struct group *group, struct bar_item *item);
void group_calculate_bounds(struct group *group);

#endif