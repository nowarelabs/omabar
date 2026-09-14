#include "context.h"
#include <CoreGraphics/CoreGraphics.h>

CGContextRef context_create(int width, int height) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        NULL,
        (size_t)width * 2, /* 2x retina */
        (size_t)height * 2,
        8,
        (size_t)width * 2 * 4,
        cs,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(cs);
    if (ctx) {
        CGContextScaleCTM(ctx, 2.0, 2.0);
    }
    return ctx;
}

void context_destroy(CGContextRef context) {
    if (context)
        CGContextRelease(context);
}