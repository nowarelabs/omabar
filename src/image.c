#include "image.h"
#include "misc/helpers.h"
#include <ImageIO/ImageIO.h>
#include <stdlib.h>

struct image *image_create(const char *path) {
    if (!path) return NULL;

    CFStringRef path_str = CFStringCreateWithCString(NULL, path,
                                                     kCFStringEncodingUTF8);
    if (!path_str) return NULL;

    CFURLRef url = CFURLCreateWithFileSystemPath(
        NULL, path_str, kCFURLPOSIXPathStyle, false);
    CFRelease(path_str);
    if (!url) return NULL;

    CGImageSourceRef src = CGImageSourceCreateWithURL(url, NULL);
    CFRelease(url);
    if (!src) return NULL;

    CGImageRef img = CGImageSourceCreateImageAtIndex(src, 0, NULL);
    CFRelease(src);
    if (!img) return NULL;

    struct image *i = calloc(1, sizeof(*i));
    i->image = img;
    i->path = strdup(path);
    return i;
}

void image_destroy(struct image *image) {
    if (!image) return;
    if (image->image) {
        CGImageRelease(image->image);
        image->image = NULL;
    }
    free(image->path);
}

void image_draw(struct image *image, struct CGContext *ctx, CGRect frame) {
    if (!image || !image->image || !ctx) return;
    CGContextDrawImage(ctx, frame, image->image);
}