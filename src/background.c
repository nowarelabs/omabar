#include "background.h"
#include "misc/helpers.h"
#include <stdlib.h>

void background_init(struct background *bg) {
    memset(bg, 0, sizeof(*bg));
    bg->color = color_from_hex(0x00000000);
    bg->border_color = color_from_hex(0xffffffff);
    bg->corner_radius = 0;
    bg->border_width = 0;
    shadow_init(&bg->shadow);
}

void background_destroy(struct background *bg) {
    if (!bg) return;
    if (bg->image) {
        image_destroy(bg->image);
        free(bg->image);
        bg->image = NULL;
    }
}

static void draw_rect(CGContextRef context,
                      CGRect region,
                      struct color *fill_color,
                      uint32_t corner_radius,
                      uint32_t line_width,
                      struct color *stroke_color) {
    CGContextSetLineWidth(context, line_width);
    if (stroke_color)
        CGContextSetRGBStrokeColor(context,
                                   stroke_color->r,
                                   stroke_color->g,
                                   stroke_color->b,
                                   stroke_color->a);
    CGContextSetRGBFillColor(context,
                             fill_color->r,
                             fill_color->g,
                             fill_color->b,
                             fill_color->a);

    CGMutablePathRef path = CGPathCreateMutable();
    CGRect inset_region = CGRectInset(region,
                                      (float)line_width / 2.f,
                                      (float)line_width / 2.f);
    if (corner_radius > inset_region.size.height / 2.f
        || corner_radius > inset_region.size.width / 2.f)
        corner_radius = inset_region.size.height > inset_region.size.width
                            ? (uint32_t)(inset_region.size.width / 2.f)
                            : (uint32_t)(inset_region.size.height / 2.f);
    CGPathAddRoundedRect(path, NULL, inset_region,
                         corner_radius, corner_radius);
    CGContextAddPath(context, path);
    CGContextDrawPath(context, kCGPathFillStroke);
    CFRelease(path);
}

static void draw_ellipse(CGContextRef context,
                         CGRect region,
                         struct color *fill_color,
                         struct color *stroke_color,
                         uint32_t line_width) {
    CGContextSetLineWidth(context, line_width);
    if (stroke_color)
        CGContextSetRGBStrokeColor(context,
                                   stroke_color->r,
                                   stroke_color->g,
                                   stroke_color->b,
                                   stroke_color->a);
    CGContextSetRGBFillColor(context,
                             fill_color->r,
                             fill_color->g,
                             fill_color->b,
                             fill_color->a);
    CGContextAddEllipseInRect(context,
                              CGRectInset(region,
                                          (float)line_width / 2.f,
                                          (float)line_width / 2.f));
    CGContextDrawPath(context, kCGPathFillStroke);
}

static void background_draw_image(struct background *bg,
                                  CGContextRef context,
                                  CGRect bounds) {
    if (!bg || !bg->image || !bg->image->image) return;

    CGContextSaveGState(context);

    if (bg->corner_radius > 0 || bg->clip) {
        CGMutablePathRef path = CGPathCreateMutable();
        if (bg->type == 1)
            CGPathAddEllipseInRect(path, NULL, bounds);
        else
            CGPathAddRoundedRect(path, NULL, bounds,
                                 bg->corner_radius, bg->corner_radius);
        CGContextAddPath(context, path);
        CGContextClip(context);
        CFRelease(path);
    }

    image_draw(bg->image, context, bounds);
    CGContextRestoreGState(context);
}

void background_draw(struct background *bg, CGContextRef ctx, CGRect bounds) {
    if (!bg || !ctx) return;

    if (bg->color.a == 0
        && (bg->border_width == 0 || bg->border_color.a == 0)
        && !bg->has_shadow
        && !(bg->image && bg->image->image))
        return;

    CGRect background_bounds = CGRectInset(bounds,
                                           bg->padding_left,
                                           bg->padding_top);
    float w = background_bounds.size.width - bg->padding_left - bg->padding_right;
    float hh = background_bounds.size.height - bg->padding_top - bg->padding_bottom;
    background_bounds.size.width = w > 0 ? w : 0;
    background_bounds.size.height = hh > 0 ? hh : 0;
    background_bounds.origin.y += bg->y_offset;

    if (bg->has_shadow) {
        CGRect shadow_bounds = shadow_get_bounds(&bg->shadow,
                                                 background_bounds);
        if (bg->type == 1) {
            draw_ellipse(ctx, shadow_bounds, &bg->shadow.color,
                         &bg->shadow.color, bg->border_width);
        } else {
            draw_rect(ctx, shadow_bounds, &bg->shadow.color,
                      bg->corner_radius, bg->border_width, &bg->shadow.color);
        }
    }

    if (bg->type == 1) {
        draw_ellipse(ctx, background_bounds, &bg->color,
                     &bg->border_color, bg->border_width);
    } else {
        draw_rect(ctx, background_bounds, &bg->color,
                  bg->corner_radius, bg->border_width, &bg->border_color);
    }

    if (bg->image && bg->image->image)
        background_draw_image(bg, ctx, background_bounds);
}