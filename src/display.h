#ifndef OMABAR_DISPLAY_H
#define OMABAR_DISPLAY_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdint.h>
#include <stdbool.h>

unsigned int display_arrangement_id(CGDirectDisplayID did);
CGRect display_bounds(CGDirectDisplayID did);
uint64_t display_space_id(CGDirectDisplayID did);
uint64_t display_space_display_id(uint64_t sid);
uint64_t *display_space_list(CGDirectDisplayID did, int *space_count);
void display_observe(int *did);
int display_begin(void);
void display_end(void);
bool display_has_notch(CGDirectDisplayID did);
uint32_t display_notch_width(CGDirectDisplayID did);
CGRect display_safe_area(CGDirectDisplayID did);

/* brightness */
void forced_brightness_event(void);
void display_brightness_begin(void);
void display_brightness_end(void);

#endif