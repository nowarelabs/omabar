#ifndef OMABAR_FONT_H
#define OMABAR_FONT_H

#include <CoreText/CoreText.h>
#include <stdbool.h>

struct font {
    CTFontRef font;
    char *family;
    char *style;
    double size;
};

struct font *font_create(const char *family, const char *style, double size);
struct font *font_create_descriptor(CTFontDescriptorRef descriptor);
void font_destroy(struct font *font);
bool font_equal(struct font *a, struct font *b);

#endif