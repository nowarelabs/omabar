#ifndef OMABAR_CONTEXT_H
#define OMABAR_CONTEXT_H

#include <CoreGraphics/CoreGraphics.h>

CGContextRef context_create(int width, int height);
void context_destroy(CGContextRef context);

#endif