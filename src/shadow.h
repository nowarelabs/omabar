#ifndef OMABAR_SHADOW_H
#define OMABAR_SHADOW_H

#include <CoreGraphics/CoreGraphics.h>

#include "color.h"

struct shadow {
    struct color color;
    double angle;
    double distance;
};

#endif