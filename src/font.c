#include "font.h"
#include "misc/helpers.h"
#include <stdlib.h>

struct font *font_create(const char *family, const char *style, double size) {
    CTFontDescriptorRef descriptor;
    CTFontRef font;

    CFStringRef family_cf = CFStringCreateWithCString(NULL, family,
                                                      kCFStringEncodingUTF8);
    CFStringRef style_cf = CFStringCreateWithCString(NULL, style,
                                                     kCFStringEncodingUTF8);

    CFMutableDictionaryRef attrs = CFDictionaryCreateMutable(
        NULL, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(attrs, kCTFontFamilyNameAttribute, family_cf);
    CFDictionarySetValue(attrs, kCTFontStyleNameAttribute, style_cf);
    CFDictionarySetValue(attrs, kCTFontSizeAttribute,
                         CFNumberCreate(NULL, kCFNumberDoubleType, &size));

    descriptor = CTFontDescriptorCreateWithAttributes(attrs);
    font = CTFontCreateWithFontDescriptor(descriptor, size, NULL);

    CFRelease(attrs);
    CFRelease(style_cf);
    CFRelease(family_cf);
    CFRelease(descriptor);

    if (!font) return NULL;

    struct font *f = calloc(1, sizeof(*f));
    f->font = font;
    f->family = strdup(family);
    f->style = strdup(style);
    f->size = size;
    return f;
}

void font_destroy(struct font *font) {
    if (!font) return;
    if (font->font) CFRelease(font->font);
    free(font->family);
    free(font->style);
    free(font);
}

bool font_equal(struct font *a, struct font *b) {
    return a == b;
}