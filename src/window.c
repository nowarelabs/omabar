#include "window.h"
#include "bar.h"
#include "bar_manager.h"
#include "layer.h"
#include "misc/extern.h"
#include "misc/helpers.h"
#include <pthread.h>
#include <dispatch/dispatch.h>
#include <stdlib.h>

uint32_t g_connection = 0;
CGPoint g_nirvana = { -4096.0f, -4096.0f };
CFTypeRef g_transaction = NULL;

static int g_space = 0;

extern struct bar_manager g_bar_manager;

#define kCGBackingStoreBuffered 2

void window_init(struct window *window) {
    window->context = NULL;
    window->surface = NULL;
    window->parent = NULL;
    window->frame = CGRectNull;
    window->id = 0;
    window->origin = CGPointZero;
    window->needs_move = false;
    window->needs_resize = false;
    window->order_mode = W_ABOVE;
    window->refc = 1;
}

void window_clear(struct window *window) {
    window->context = NULL;
    window->surface = NULL;
    window->parent = NULL;
    window->id = 0;
    window->origin = CGPointZero;
    window->frame = CGRectNull;
    window->needs_move = false;
    window->needs_resize = false;
    window->order_mode = W_ABOVE;
    window->refc = 1;
}

uint32_t window_connection(void) {
    if (g_connection == 0)
        g_connection = SLSMainConnectionID();
    return g_connection;
}

static CFTypeRef window_create_region(struct window *window) {
    CFTypeRef frame_region = NULL;
    CGRect rect = { { 0, 0 },
                    { window->frame.size.width, window->frame.size.height } };
    CGSNewRegionWithRect(&rect, &frame_region);
    return frame_region;
}

static void window_clear_background(struct window *window) {
    if (window->context) {
        CGContextClearRect(window->context, window->frame);
        CGContextFlush(window->context);
    }
}

struct window *window_open(struct bar *bar, int width, int height) {
    (void)bar;

    struct window *window = calloc(1, sizeof(*window));
    if (!window) return NULL;
    window_init(window);

    window->frame = CGRectMake(0, 0, width, height);
    window->origin = CGPointZero;

    uint64_t set_tags = kCGSExposeFadeTagBit | kCGSPreventsActivationTagBit;
    uint64_t clear_tags = 0;

    CFTypeRef frame_region = window_create_region(window);
    CFTypeRef empty_region = CGRegionCreateEmptyRegion();
    uint32_t id = 0;

    if (!frame_region || !empty_region) {
        if (frame_region) CFRelease(frame_region);
        if (empty_region) CFRelease(empty_region);
        free(window);
        return NULL;
    }

    SLSNewWindowWithOpaqueShapeAndContext(g_connection,
                                          kCGBackingStoreBuffered,
                                          frame_region,
                                          empty_region,
                                          13 | (1 << 18),
                                          &set_tags,
                                          window->origin.x,
                                          window->origin.y,
                                          64,
                                          &id,
                                          NULL);
    CFRelease(empty_region);
    CFRelease(frame_region);

    if (!id) {
        free(window);
        return NULL;
    }
    window->id = id;

    SLSSetWindowResolution(g_connection, window->id, 2.0f);
    SLSSetWindowTags(g_connection, window->id, &set_tags, 64);
    SLSClearWindowTags(g_connection, window->id, &clear_tags, 64);
    SLSSetWindowOpacity(g_connection, window->id, 0);

    window->context = SLWindowContextCreate(g_connection, window->id, NULL);
    window_clear_background(window);
    CGContextSetInterpolationQuality(window->context, kCGInterpolationNone);

    window->needs_move = false;
    window->needs_resize = false;

    if (g_bar_manager.sticky) {
        if (!g_space) {
            g_space = (int)SLSSpaceCreate(g_connection, 1, 0);
            SLSSpaceSetAbsoluteLevel(g_connection, g_space, 0);

            CFArrayRef space_list = cfarray_of_cfnumbers(&g_space,
                                                         sizeof(uint32_t),
                                                         1,
                                                         kCFNumberSInt32Type);
            SLSShowSpaces(g_connection, space_list);
            CFRelease(space_list);
        }

        CFArrayRef window_list = cfarray_of_cfnumbers(&window->id,
                                                      sizeof(uint32_t),
                                                      1,
                                                      kCFNumberSInt32Type);
        SLSSpaceAddWindowsAndRemoveFromSpaces(g_connection,
                                              g_space,
                                              window_list,
                                              0x7);
        CFRelease(window_list);
    }

    return window;
}

void window_close(struct window *window) {
    if (!window || !window->id) return;

    SLSOrderWindow(g_connection, window->id, 0, 0);
    surface_destroy(window->surface);
    if (window->context) CGContextRelease(window->context);
    SLSReleaseWindow(g_connection, window->id);
    window_clear(window);
}

void window_destroy(struct window *window) {
    if (!window) return;
    if (--window->refc > 0) return;
    window_close(window);
    free(window);
}

void window_flush(struct window *window) {
    if (!window || !window->context) return;
    CGContextFlush(window->context);
}

void windows_freeze(void) {
    if (g_transaction) return;

    if (__builtin_available(macOS 26.0, *)) { }
    else SLSDisableUpdate(g_connection);

    g_transaction = SLSTransactionCreate(g_connection);
}

void windows_unfreeze(void) {
    if (!g_transaction) return;

    SLSTransactionCommit(g_transaction, 0x0);
    CFRelease(g_transaction);
    g_transaction = NULL;

    if (__builtin_available(macOS 26.0, *)) { }
    else SLSReenableUpdate(g_connection);
}

void window_move(struct window *window, CGPoint point) {
    window->origin = point;

    if (__builtin_available(macOS 12.0, *)) {
        windows_freeze();
        SLSTransactionMoveWindowWithGroup(g_transaction, window->id, point);
    } else {
        SLSMoveWindow(g_connection, window->id, &point);

        CFNumberRef number = CFNumberCreate(NULL,
                                            kCFNumberSInt32Type,
                                            &window->id);
        const void *values[1] = { number };
        CFArrayRef array = CFArrayCreate(NULL, values, 1,
                                         &kCFTypeArrayCallBacks);
        SLSReassociateWindowsSpacesByGeometry(g_connection, array);
        CFRelease(array);
        CFRelease(number);
    }
}

void window_set_frame(struct window *window, int x, int y, int width, int height) {
    if (!window) return;

    CGRect frame = CGRectMake(x, y, width, height);
    if (window->needs_move
        || !CGPointEqualToPoint(window->origin, frame.origin)) {
        window->needs_move = true;
        window->origin = frame.origin;
    }

    if (window->needs_resize
        || !CGSizeEqualToSize(window->frame.size, frame.size)) {
        window->needs_resize = true;
        window->frame.size = frame.size;
    }
}

void window_set_origin(struct window *window, int x, int y) {
    if (!window) return;
    window->needs_move = true;
    window->origin = CGPointMake(x, y);
}

static void window_defer_update(struct window *window) {
    void (^block)(void) = ^{
        if (!window->context) return;
        if (--window->refc <= 0) window_destroy(window);
        else {
            window_clear_background(window);
            window_flush(window);
        }
    };

    if (pthread_main_np()) dispatch_async(dispatch_get_main_queue(), block);
    else dispatch_sync(dispatch_get_main_queue(), block);
}

bool window_apply_frame(struct window *window, bool forced) {
    if (!window) return false;

    if (window->needs_resize || forced) {
        windows_freeze();
        CFTypeRef frame_region = window_create_region(window);
        window->refc++;

        if (__builtin_available(macOS 26.0, *)) {
            SLSTransactionSetWindowShape(g_transaction,
                                         window->id,
                                         window->origin.x,
                                         window->origin.y,
                                         frame_region);
            window_move(window, window->origin);
            if (SBSLSTransactionAddPostDecodeAction) {
                SBSLSTransactionAddPostDecodeAction(g_transaction, ^{
                    window_defer_update(window);
                });
            } else window_defer_update(window);
        }
        else if (__builtin_available(macOS 13.0, *)) {
            SLSSetWindowShape(g_connection,
                              window->id,
                              g_nirvana.x,
                              g_nirvana.y,
                              frame_region);
            window_clear_background(window);
            window_move(window, window->origin);
            window_defer_update(window);
        } else {
            SLSSetWindowShape(g_connection, window->id, 0, 0, frame_region);
            window_clear_background(window);
            window_move(window, window->origin);
            window_defer_update(window);
        }

        if (window->surface)
            surface_resize(window->surface,
                           (int)window->frame.size.width,
                           (int)window->frame.size.height);
        if (frame_region) CFRelease(frame_region);

        window->needs_move = false;
        window->needs_resize = false;
        return true;
    } else if (window->needs_move) {
        window_move(window, window->origin);
        window->needs_move = false;
        return false;
    }
    return false;
}

void window_send_to_space(struct window *window, uint64_t dsid) {
    CFArrayRef window_list = cfarray_of_cfnumbers(&window->id,
                                                  sizeof(uint32_t),
                                                  1,
                                                  kCFNumberSInt32Type);

    SLSMoveWindowsToManagedSpace(g_connection, window_list, dsid);
    if (CGPointEqualToPoint(window->origin, g_nirvana))
        SLSMoveWindow(g_connection, window->id, &g_nirvana);
    CFRelease(window_list);
}

void window_set_level(struct window *window, uint32_t level) {
    windows_freeze();
    if (__builtin_available(macOS 14.0, *)) {
        SLSTransactionSetWindowLevel(g_transaction, window->id, level);
    } else {
        SLSSetWindowLevel(g_connection, window->id, level);
    }
}

void window_order(struct window *window, int order, int relative_to) {
    if (!window) return;
    windows_freeze();
    if (order != W_OUT) window->order_mode = order;

    if (__builtin_available(macOS 14.0, *)) {
        SLSTransactionOrderWindow(g_transaction,
                                  window->id,
                                  order,
                                  relative_to);
    } else {
        SLSOrderWindow(g_connection, window->id, order, relative_to);
    }
}

void window_assign_mouse_tracking_area(struct window *window, CGRect rect) {
    SLSRemoveAllTrackingAreas(g_connection, window->id);
    SLSAddTrackingRect(g_connection, window->id, rect);
}

void window_set_blur_radius(struct window *window, uint32_t blur_radius) {
    SLSSetWindowBackgroundBlurRadius(g_connection, window->id, blur_radius);
    if (window->context) window_clear_background(window);
}

void window_disable_shadow(struct window *window) {
    CFIndex shadow_density = 0;
    CFNumberRef shadow_density_cf = CFNumberCreate(kCFAllocatorDefault,
                                                   kCFNumberCFIndexType,
                                                   &shadow_density);

    const void *keys[1] = { CFSTR("com.apple.WindowShadowDensity") };
    const void *values[1] = { shadow_density_cf };
    CFDictionaryRef shadow_props_cf = CFDictionaryCreate(NULL,
                                                         keys,
                                                         values,
                                                         1,
                                                         &kCFTypeDictionaryKeyCallBacks,
                                                         &kCFTypeDictionaryValueCallBacks);

    SLSWindowSetShadowProperties(window->id, shadow_props_cf);
    CFRelease(shadow_density_cf);
    CFRelease(shadow_props_cf);
}