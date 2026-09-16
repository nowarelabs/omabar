#ifndef OMABAR_DISPLAY_NSSCREEN_H
#define OMABAR_DISPLAY_NSSCREEN_H

#include <CoreGraphics/CoreGraphics.h>

CGRect display_nsscreen_safe_area(CGDirectDisplayID did);
double display_nsscreen_top_inset(CGDirectDisplayID did);

#endif