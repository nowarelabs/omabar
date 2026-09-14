#ifndef OMABAR_BAR_H
#define OMABAR_BAR_H

#include "window.h"
#include "background.h"

struct bar {
    struct window *window;
    uint64_t sid;
    uint64_t dsid;
    unsigned int did;
    int adid;
    int shown;
    int hidden;
    int mouse_over;

    struct background background;
    double x_offset;
};

struct bar *bar_create(unsigned int did);
void bar_destroy(struct bar *bar);
void bar_calculate_bounds(struct bar *bar);
void bar_draw(struct bar *bar);

#endif