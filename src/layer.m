#include "layer.h"
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <stdlib.h>

/* CAContext is a QuartzCore private class with no headers and no
   linkable symbols; resolve it through the ObjC runtime so the
   binary links on any macOS release. */

static id ca_context_create(uint32_t cid, NSDictionary *options) {
    Class cls = objc_getClass("CAContext");
    if (!cls) return NULL;
    SEL sel = NSSelectorFromString(@"contextWithCGSConnection:options:");
    if (!sel) return NULL;
    id (*msg)(Class, SEL, uint32_t, id) = (void *)objc_msgSend;
    return [msg(cls, sel, cid, options) retain];
}

static void ca_context_set_layer(id ctx, id layer) {
    SEL sel = NSSelectorFromString(@"setLayer:");
    if (!sel) return;
    void (*msg)(id, SEL, id) = (void *)objc_msgSend;
    msg(ctx, sel, layer);
}

static uint32_t ca_context_id(id ctx) {
    SEL sel = NSSelectorFromString(@"contextId");
    if (!sel) return 0;
    uint32_t (*msg)(id, SEL) = (void *)objc_msgSend;
    return msg(ctx, sel);
}

struct layer *layer_create(uint32_t cid) {
    struct layer *l = calloc(1, sizeof(*l));
    if (!l) return NULL;

    CALayer *calayer = [[CALayer layer] retain];
    if (!calayer) { free(l); return NULL; }

    calayer.anchorPoint = CGPointZero;
    calayer.position = CGPointZero;
    calayer.contentsScale = 2.0;
    calayer.opaque = NO;

    id ctx = ca_context_create(cid, nil);
    if (!ctx) { [calayer release]; free(l); return NULL; }

    ca_context_set_layer(ctx, calayer);

    l->context = ctx;
    l->root = calayer;
    l->caid = ca_context_id(ctx);
    return l;
}

void layer_destroy(struct layer *layer) {
    if (!layer) return;
    if (layer->context) {
        id ctx = (id)layer->context;
        SEL sel = NSSelectorFromString(@"invalidate");
        if (sel) {
            void (*invalidate)(id, SEL) = (void *)objc_msgSend;
            invalidate(ctx, sel);
        }
        [ctx release];
    }
    if (layer->root)
        [(CALayer *)layer->root release];
    layer->context = NULL;
    layer->root = NULL;
    free(layer);
}

void layer_set_contents(struct layer *layer, CGImageRef image) {
    if (!layer || !layer->root) return;
    CALayer *calayer = (CALayer *)layer->root;
    [CATransaction setDisableActions:YES];
    calayer.contentsScale = 2.0;
    calayer.contents = (id)image;
}

void layer_set_alpha(struct layer *layer, float alpha) {
    if (!layer || !layer->root) return;
    [(CALayer *)layer->root setOpacity:alpha];
}

void layer_set_bounds(struct layer *layer, CGRect bounds) {
    if (!layer || !layer->root) return;
    CALayer *calayer = (CALayer *)layer->root;
    [CATransaction setDisableActions:YES];
    calayer.bounds = bounds;
}