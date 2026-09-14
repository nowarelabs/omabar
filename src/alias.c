#include "alias.h"
#include "misc/helpers.h"
#include <stdlib.h>

struct alias *alias_create(void) {
    struct alias *a = calloc(1, sizeof(*a));
    if (!a) return NULL;
    a->corner_radius = 0;
    a->disable_shadows = 1;
    return a;
}

void alias_destroy(struct alias *alias) {
    if (!alias) return;
    free(alias->target_pid);
    free(alias);
}

void alias_draw(struct alias *alias, struct CGContext *ctx, CGRect frame) {
    (void)alias; (void)ctx; (void)frame;
}