#include "text.h"
#include "misc/helpers.h"
#include "misc/extern.h"
#include <stdlib.h>

void text_init(struct text *text) {
    memset(text, 0, sizeof(*text));
    text->color = color_from_hex(0xffffffff);
    text->highlight_color = color_from_hex(0xffffffff);
    text->background_color = color_from_hex(0x00000000);
    text->has_shadow = 0;
    text->has_background = 0;
    text->x_offset = 0;
    text->y_offset = 0;
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
    text->line.ascent = 0;
    text->line.descent = 0;
    text->line.width = 0;

    if (!text->string || !text->font || !text->font->ct_font) return;

    CFStringRef str = CFStringCreateWithCString(NULL, text->string,
                                                kCFStringEncodingUTF8);
    if (!str) return;

    CFMutableAttributedStringRef attr =
        CFAttributedStringCreateMutable(NULL, 0);
    CFAttributedStringReplaceString(attr, CFRangeMake(0, 0), str);

    CFRange full = CFRangeMake(0, CFStringGetLength(str));
    CFAttributedStringSetAttribute(attr, full,
                                   kCTFontAttributeName, text->font->ct_font);
    CFAttributedStringSetAttribute(attr, full,
                                   kCTForegroundColorFromContextAttributeName,
                                   kCFBooleanTrue);

    CTLineRef line = CTLineCreateWithAttributedString(attr);

    text->line.line = line;
    if (line) {
        double asc = 0, desc = 0, leading = 0;
        text->line.width = (float)CTLineGetTypographicBounds(line,
                                                             &asc,
                                                             &desc,
                                                             &leading);
        text->line.ascent = (float)asc;
        text->line.descent = (float)desc;
    }
    CFRelease(attr);
    CFRelease(str);
}

/* Measure a string in a font without disturbing any struct text. Callers that
   size layout around several candidate strings (the space chips pick their
   width from the widest label) need this: routing them through text_set_string
   would leave the shared line state holding whichever label was measured
   last, making the result depend on call order. */
float text_measure(struct font *font, const char *str) {
    if (!font || !font->ct_font || !str) return 0.0f;

    CFStringRef cfstr = CFStringCreateWithCString(NULL, str,
                                                  kCFStringEncodingUTF8);
    if (!cfstr) return 0.0f;

    CFMutableAttributedStringRef attr = CFAttributedStringCreateMutable(NULL, 0);
    CFAttributedStringReplaceString(attr, CFRangeMake(0, 0), cfstr);
    CFRange full = CFRangeMake(0, CFStringGetLength(cfstr));
    CFAttributedStringSetAttribute(attr, full, kCTFontAttributeName,
                                   font->ct_font);
    CFAttributedStringSetAttribute(attr, full,
                                   kCTForegroundColorFromContextAttributeName,
                                   kCFBooleanTrue);

    CTLineRef line = CTLineCreateWithAttributedString(attr);
    float width = 0.0f;
    if (line) {
        width = (float)CTLineGetTypographicBounds(line, NULL, NULL, NULL);
        CFRelease(line);
    }
    CFRelease(attr);
    CFRelease(cfstr);
    return width;
}

void text_set_string(struct text *text, const char *str) {
    if (!text) return;    if (text->string && str && strcmp(text->string, str) == 0) return;
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

void text_set_highlight_color(struct text *text, struct color color) {
    text->highlight_color = color;
}

void text_set_highlighted(struct text *text, int on) {
    if (!text) return;
    text->highlighted = on ? 1 : 0;
}

void text_set_background(struct text *text, int enabled, struct color color) {
    if (!text) return;
    text->has_background = enabled;
    text->background_color = color;
}

void text_set_shadow(struct text *text, int enabled) {
    if (text) text->has_shadow = enabled;
}

static CGRect text_line_bounds(struct text *text, CGRect frame) {
    CGRect bounds = {
        { frame.origin.x + text->x_offset, frame.origin.y + text->y_offset },
        { text->line.width < frame.size.width ? text->line.width
                                              : frame.size.width,
          text->line.ascent + text->line.descent }
    };
    return bounds;
}

void text_draw(struct text *text, CGContextRef ctx, CGRect frame) {
    if (!text || !text->line.line || !ctx) return;

    CGContextSaveGState(ctx);
    CGContextClipToRect(ctx, frame);

    if (text->has_background && color_is_valid(text->background_color)) {
        CGRect bg = text_line_bounds(text, frame);
        CGContextSetRGBFillColor(ctx,
                                 text->background_color.r,
                                 text->background_color.g,
                                 text->background_color.b,
                                 text->background_color.a);
        CGContextFillRect(ctx, bg);
    }

    CGFloat x = frame.origin.x + text->x_offset;
    /* bar_draw() works top-down, so frame.origin.y is the top of the line box
       and the baseline sits one ascent below it. Anchoring on descent (as a
       y-up layout would) floats the text above the box it was measured for. */
    CGFloat y = frame.origin.y + text->y_offset + text->line.ascent;

    /* while highlighted the glyph is drawn in highlight_color, so a selected
       chip on a light fill can carry dark text */
    const struct color *fg = &text->color;
    if (text->highlighted && color_is_valid(text->highlight_color))
        fg = &text->highlight_color;

    /* CoreText lays glyphs out along a y-up baseline, but bar_draw() flips
       the context once so that every other draw call can work top-down.
       Drawing straight into that flipped context renders every glyph
       mirrored top-to-bottom, so undo the flip here, anchored at the
       baseline. Inside this local space +x is right and +y is *down* on
       screen, which is the opposite of CoreText's own sense. */
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, x, y);
    CGContextScaleCTM(ctx, 1.0, -1.0);

    if (text->has_shadow) {
        CGContextSetRGBFillColor(ctx, 0.0f, 0.0f, 0.0f,
                                 0.5f * fg->a);
        CGContextSetTextPosition(ctx, 1.0f, 1.0f);
        CTLineDraw(text->line.line, ctx);
    }

    CGContextSetRGBFillColor(ctx,
                             fg->r,
                             fg->g,
                             fg->b,
                             fg->a);
    CGContextSetTextPosition(ctx, 0.0f, 0.0f);
    CTLineDraw(text->line.line, ctx);

    CGContextRestoreGState(ctx);
    CGContextRestoreGState(ctx);
}