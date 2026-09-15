#include "image.h"
#include "misc/helpers.h"
#include <ImageIO/ImageIO.h>
#include <stdlib.h>

static CGImageRef image_load(const char *path) {
    if (!path) return NULL;

    CFStringRef path_str = CFStringCreateWithCString(NULL, path,
                                                     kCFStringEncodingUTF8);
    if (!path_str) return NULL;

    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, path_str,
                                                 kCFURLPOSIXPathStyle, false);
    CFRelease(path_str);
    if (!url) return NULL;

    CGImageSourceRef src = CGImageSourceCreateWithURL(url, NULL);
    CFRelease(url);
    if (!src) return NULL;

    CGImageRef img = CGImageSourceCreateImageAtIndex(src, 0, NULL);
    CFRelease(src);
    return img;
}

struct image *image_create(const char *path) {
    struct image *image = calloc(1, sizeof(*image));
    if (!image) return NULL;
    image->border_color = color_from_hex(0xffffffff);
    image->image = image_load(path);
    if (!image->image && path) {
        free(image);
        return NULL;
    }
    image->path = path ? strdup(path) : NULL;
    return image;
}

bool image_set_path(struct image *image, const char *path) {
    if (!image || !path) return false;
    if (image->path && strcmp(image->path, path) == 0) return false;

    CGImageRef new_image = image_load(path);
    if (!new_image) return false;

    if (image->image) CGImageRelease(image->image);
    image->image = new_image;
    free(image->path);
    image->path = strdup(path);
    return true;
}

void image_destroy(struct image *image) {
    if (!image) return;
    if (image->image) {
        CGImageRelease(image->image);
        image->image = NULL;
    }
    free(image->path);
}

void image_draw(struct image *image, CGContextRef ctx, CGRect frame) {
    if (!image || !image->image || !ctx) return;

    CGContextSaveGState(ctx);

    if (image->corner_radius > 0) {
        CGMutablePathRef path = CGPathCreateMutable();
        CGPathAddRoundedRect(path, NULL, frame,
                             image->corner_radius, image->corner_radius);
        CGContextAddPath(ctx, path);
        CGContextClip(ctx);
        CFRelease(path);
    }

    CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
    CGContextDrawImage(ctx, frame, image->image);

    if (image->border_width > 0 && image->border_color.a > 0) {
        CGRect inset = CGRectInset(frame,
                                   (float)image->border_width / 2.f,
                                   (float)image->border_width / 2.f);
        CGContextSetLineWidth(ctx, image->border_width);
        CGContextSetRGBStrokeColor(ctx,
                                   image->border_color.r,
                                   image->border_color.g,
                                   image->border_color.b,
                                   image->border_color.a);
        if (image->corner_radius > 0) {
            CGMutablePathRef path = CGPathCreateMutable();
            CGPathAddRoundedRect(path, NULL, inset,
                                 image->corner_radius, image->corner_radius);
            CGContextAddPath(ctx, path);
            CGContextStrokePath(ctx);
            CFRelease(path);
        } else {
            CGContextStrokeRect(ctx, inset);
        }
    }

    CGContextRestoreGState(ctx);
}