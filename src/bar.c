#include "bar.h"
#include "bar_manager.h"
#include "display.h"
#include "misc/helpers.h"
#include <stdlib.h>

extern struct bar_manager g_bar_manager;

struct bar *bar_create(unsigned int did) {
    (void)did;
    return NULL;
}

void bar_destroy(struct bar *bar) {
    if (!bar) return;
    window_close(bar->window);
    free(bar);
}

void bar_calculate_bounds(struct bar *bar) {
    (void)bar;
}

void bar_draw(struct bar *bar) {
    (void)bar;
}