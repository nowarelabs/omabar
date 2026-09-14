#include "text.h"
#include "misc/helpers.h"
#include "misc/extern.h"
#include <stdlib.h>

void text_init(struct text *text) {
    memset(text, 0, sizeof(*text));
    text->color = color_from_hex(0xffffffff);
    text->highlight_color = color_from_hex(0xffffffff);
    text->has_shadow = 0;
}

void text_destroy(struct text *text) {
    if (!text) return;
    if (text->line.line) {
        CFRelease(text->line.line);
        text->line.line = NULL;
    }
    free(text->string);
    text->string = NULL;
}

static void text_recreate_line(struct text *text) {
    if (text->line.line) {
        CFRelease(text->line.line);
        text->line.line = NULL;
    }
    if (!text->string || !text->font) return;

    CFStringRef str = CFStringCreateWithCString(NULL, text->string,
                                                kCFStringEncodingUTF8);
    if (!str) return;

    CFMutableAttributedStringRef attr =
        CFAttributedStringCreateMutable(NULL, 0);
    CFAttributedStringReplaceString(attr, CFRangeMake(0, 0), str);

    CFAttributedStringSetAttribute(attr, CFRangeMake(0, CFStringGetLength(str)),
                                   kCTFontAttributeName, text->font->font);
    CFAttributedStringSetAttribute(attr, CFRangeMake(0, CFStringGetLength(str)),
                                   kCTForegroundColorFromContextAttributeName,
                                   kCFBooleanTrue);

    CTLineRef line = CTLineCreateWithAttributedString(attr);

    text->line.line = line;
    text->line.ascent = 0;
    text->line.descent = 0;
    if (line) {
        double asc = 0, desc = 0, leading = 0;
        CTLineGetTypographicBounds(line, &asc, &desc, &leading);
        text->line.ascent = asc;
        text->line.descent = desc;
    }
    CFRelease(attr);
    CFRelease(str);
}

void text_set_string(struct text *text, const char *str) {
    if (!text) return;
    if (text->string && str && strcmp(text->string, str) == 0) return;
    free(text->string);
    text->string = str ? strdup(str) : NULL;
    text_recreate_line(text);
}

void text_set_font(struct text *text, struct font *font) {
    if (text->font == font) return;
    text->font = font;
    text_recreate_line(text);
}

void text_set_color(struct text *text, struct color color) {
    text->color = color;
}

void text_draw(struct text *text, struct CGContext *ctx, CGRect frame) {
    (void)text; (void)ctx; (void)frame;
}