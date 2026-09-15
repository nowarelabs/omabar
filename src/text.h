#ifndef OMABAR_TEXT_H
#define OMABAR_TEXT_H

#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <stdbool.h>

#include "font.h"
#include "color.h"

struct text_line {
    CTLineRef line;
    float ascent;
    float descent;
    float width;
};

struct text {
    struct text_line line;
    struct font *font;
    struct color color;
    struct color highlight_color;

    char *string;
    int has_shadow;
    int has_background;
    struct color background_color;
    int scroll_enabled;
    double scroll_duration;
    int y_offset;
    int x_offset;
};

void text_init(struct text *text);
void text_destroy(struct text *text);
void text_set_string(struct text *text, const char *str);
void text_set_font(struct text *text, struct font *font);
void text_set_color(struct text *text, struct color color);
void text_set_highlight_color(struct text *text, struct color color);
void text_set_background(struct text *text, int enabled, struct color color);
void text_set_shadow(struct text *text, int enabled);
void text_draw(struct text *text, CGContextRef ctx, CGRect frame);

#endif