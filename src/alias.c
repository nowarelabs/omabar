#include "alias.h"
#include "misc/extern.h"
#include "misc/helpers.h"
#include "window.h"
#include <CoreFoundation/CoreFoundation.h>
#include <stdlib.h>
#include <string.h>

struct alias *alias_create(void) {
    struct alias *a = calloc(1, sizeof(*a));
    if (!a) return NULL;
    a->background_color = color_from_hex(0x00000000);
    a->update_frequency = 1;
    a->corner_radius = 0;
    a->window = CGRectNull;
    return a;
}

void alias_destroy(struct alias *alias) {
    if (!alias) return;
    if (alias->image) {
        CGImageRelease(alias->image);
        alias->image = NULL;
    }
    free(alias->owner);
    free(alias->name);
    free(alias->target_pid);
    free(alias);
}

void alias_set_target(struct alias *alias, const char *owner, const char *name) {
    if (!alias) return;
    free(alias->owner);
    free(alias->name);
    alias->owner = owner ? strdup(owner) : NULL;
    alias->name = name ? strdup(name) : NULL;
    alias->window = CGRectNull;
}

bool alias_request_permission(struct alias *alias) {
    if (!alias) return false;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
    if (__builtin_available(macOS 11.0, *)) {
        alias->permission = CGRequestScreenCaptureAccess();
        return alias->permission;
    }
#endif
    CFArrayRef list = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly,
                                                 kCGNullWindowID);
    if (!list || CFArrayGetCount(list) == 0) {
        if (list) CFRelease(list);
        alias->permission = false;
        return false;
    }
    CFDictionaryRef dict = CFArrayGetValueAtIndex(list, 0);
    CFNumberRef wid_ref = dict ? CFDictionaryGetValue(dict, kCGWindowNumber)
                               : NULL;
    uint64_t wid = 0;
    if (wid_ref) CFNumberGetValue(wid_ref, kCFNumberLongLongType, &wid);
    CFRelease(list);

    CGImageRef img = NULL;
    if (wid) {
        SLSCaptureWindowsContentsToRectWithOptions(window_connection(), &wid,
                                                   true, CGRectMake(0, 0, 1, 1),
                                                   1 << 8, &img);
        if (img) CFRelease(img);
    }
    alias->permission = img != NULL;
    return alias->permission;
}

static uint64_t alias_find_window(struct alias *alias) {
    CFArrayRef list = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly,
                                                 kCGNullWindowID);
    if (!list) return 0;

    uint64_t found = 0;
    CFIndex count = CFArrayGetCount(list);
    for (CFIndex i = 0; i < count && !found; i++) {
        CFDictionaryRef dict = CFArrayGetValueAtIndex(list, i);
        if (!dict) continue;

        CFStringRef owner_ref = CFDictionaryGetValue(dict, kCGWindowOwnerName);
        CFStringRef name_ref = CFDictionaryGetValue(dict, kCGWindowName);
        CFNumberRef layer_ref = CFDictionaryGetValue(dict, kCGWindowLayer);
        CFDictionaryRef bounds_ref = CFDictionaryGetValue(dict, kCGWindowBounds);
        CFNumberRef wid_ref = CFDictionaryGetValue(dict, kCGWindowNumber);
        if (!owner_ref || !name_ref || !layer_ref || !bounds_ref || !wid_ref)
            continue;

        long long layer = 0;
        CFNumberGetValue(layer_ref, kCFNumberLongLongType, &layer);
        if (layer != MENUBAR_LAYER) continue;

        CGRect bounds = CGRectNull;
        if (!CGRectMakeWithDictionaryRepresentation(bounds_ref, &bounds))
            continue;

        char *owner = cfstring_copy(owner_ref);
        if (string_equals(owner, "Window Server")) {
            free(owner);
            continue;
        }

        if (alias->owner && strcmp(alias->owner, owner) != 0) {
            free(owner);
            continue;
        }
        free(owner);

        if (alias->name && alias->name[0] != '\0') {
            char *name = cfstring_copy(name_ref);
            bool match = strcmp(alias->name, name) == 0;
            free(name);
            if (!match) continue;
        }

        CFNumberGetValue(wid_ref, kCFNumberLongLongType, &found);
        alias->window = bounds;
    }
    CFRelease(list);
    return found;
}

static CGImageRef alias_capture_window(uint64_t wid, CGRect bounds) {
    CGImageRef image = NULL;
    SLSCaptureWindowsContentsToRectWithOptions(window_connection(),
                                               &wid, true, bounds,
                                               1 << 8, &image);
    return image;
}

bool alias_update_image(struct alias *alias, bool forced) {
    (void)forced;
    if (!alias) return false;

    uint64_t wid = alias_find_window(alias);
    if (wid == 0) {
        if (alias->image) {
            CGImageRelease(alias->image);
            alias->image = NULL;
        }
        return false;
    }

    CGImageRef captured = alias_capture_window(wid, alias->window);
    if (!captured) {
        if (alias->image) {
            CGImageRelease(alias->image);
            alias->image = NULL;
        }
        return false;
    }

    if (alias->image) CGImageRelease(alias->image);
    alias->image = captured;
    return true;
}

bool alias_update(struct alias *alias, bool forced) {
    if (!alias || alias->update_frequency == 0) return false;
    alias->counter++;
    if (forced || alias->counter >= alias->update_frequency) {
        alias->counter = 0;
        return alias_update_image(alias, forced);
    }
    return false;
}

void alias_draw(struct alias *alias, CGContextRef ctx, CGRect frame) {
    if (!alias || !ctx) return;

    CGContextSaveGState(ctx);

    if (alias->corner_radius > 0) {
        CGMutablePathRef p = CGPathCreateMutable();
        CGPathAddRoundedRect(p, NULL, frame,
                             alias->corner_radius, alias->corner_radius);
        CGContextAddPath(ctx, p);
        CGContextClip(ctx);
        CFRelease(p);
    }

    if (alias->image) {
        CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
        CGContextDrawImage(ctx, frame, alias->image);

        if (alias->inverse) {
            CGContextClipToRect(ctx, frame);
            CGContextSetBlendMode(ctx, kCGBlendModeDifference);
            CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
            CGContextFillRect(ctx, frame);
        }
    }
    else {
        CGContextSetRGBFillColor(ctx,
                                 alias->background_color.r,
                                 alias->background_color.g,
                                 alias->background_color.b,
                                 alias->background_color.a);
        CGContextFillRect(ctx, frame);
    }

    CGContextRestoreGState(ctx);
}