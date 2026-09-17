#include "alias.h"
#include "misc/extern.h"
#include "misc/helpers.h"
#include "window.h"
#include "display.h"
#include "app_windows.h"
#include <CoreFoundation/CoreFoundation.h>
#include <stdlib.h>
#include <string.h>

#define HW_CAPTURE_OPAQUE 2

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
    free(alias->bundle_id);
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

void alias_set_bundle_id(struct alias *alias, const char *bundle_id) {
    if (!alias) return;
    if (alias->bundle_id && bundle_id &&
        strcmp(alias->bundle_id, bundle_id) == 0)
        return;
    free(alias->bundle_id);
    alias->bundle_id = bundle_id ? strdup(bundle_id) : NULL;
    if (alias->bundle_id) {
        free(alias->target_pid);
        alias->target_pid = NULL;
    }
    alias->window = CGRectNull;
}

void alias_set_target_pid(struct alias *alias, const char *target_pid) {
    if (!alias) return;
    if (alias->target_pid && target_pid &&
        strcmp(alias->target_pid, target_pid) == 0)
        return;
    free(alias->target_pid);
    alias->target_pid = target_pid ? strdup(target_pid) : NULL;
    alias->window = CGRectNull;
}

void alias_set_size(struct alias *alias, int width, int height) {
    if (!alias) return;
    if (width > 0) alias->width = width;
    if (height > 0) alias->height = height;
    if (alias->update_frequency == 0)
        alias->update_frequency = 1;
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

/* Resolve the capture target to a PID: explicit target_pid wins, then the
   bundle identifier (task 89), then nothing (owner/name matching below). */
static pid_t alias_resolved_pid(struct alias *alias) {
    if (!alias) return 0;
    if (alias->target_pid && alias->target_pid[0] != '\0') {
        pid_t pid = (pid_t)strtol(alias->target_pid, NULL, 10);
        return pid > 0 ? pid : 0;
    }
    if (alias->bundle_id && alias->bundle_id[0] != '\0')
        return app_windows_pid_for_bundle_id(alias->bundle_id);
    return 0;
}

static uint64_t alias_find_window(struct alias *alias) {
    pid_t target_pid = alias_resolved_pid(alias);

    CFArrayRef list = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly,
                                                 kCGNullWindowID);
    if (!list) return 0;

    uint64_t found = 0;
    CFIndex count = CFArrayGetCount(list);
    for (CFIndex i = 0; i < count && !found; i++) {
        CFDictionaryRef dict = CFArrayGetValueAtIndex(list, i);
        if (!dict) continue;

        CFNumberRef layer_ref = CFDictionaryGetValue(dict, kCGWindowLayer);
        CFNumberRef bounds_ref = CFDictionaryGetValue(dict, kCGWindowBounds);
        CFNumberRef wid_ref = CFDictionaryGetValue(dict, kCGWindowNumber);
        if (!layer_ref || !bounds_ref || !wid_ref) continue;

        long long layer = 0;
        CFNumberGetValue(layer_ref, kCFNumberLongLongType, &layer);
        if (layer != MENUBAR_LAYER) continue;

        CGRect bounds = CGRectNull;
        if (!CGRectMakeWithDictionaryRepresentation(bounds_ref, &bounds))
            continue;

        /* PID-targeted match (bundle id or explicit target_pid). */
        if (target_pid > 0) {
            CFNumberRef pid_ref = CFDictionaryGetValue(dict, kCGWindowOwnerPID);
            if (!pid_ref) continue;
            long long owner_pid = 0;
            CFNumberGetValue(pid_ref, kCFNumberLongLongType, &owner_pid);
            if (owner_pid != (long long)target_pid) continue;
        } else {
            CFStringRef owner_ref = CFDictionaryGetValue(dict, kCGWindowOwnerName);
            CFStringRef name_ref = CFDictionaryGetValue(dict, kCGWindowName);
            if (!owner_ref || !name_ref) continue;

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
        }

        CFNumberGetValue(wid_ref, kCFNumberLongLongType, &found);
        alias->window = bounds;
    }
    CFRelease(list);
    return found;
}

static CGImageRef alias_capture_window(struct alias *alias, uint64_t wid,
                                       CGRect bounds) {
    if (CGRectIsNull(bounds) || CGRectIsEmpty(bounds)) return NULL;

    /* Primary path (task 89): hardware capture of the space, cropped to the
       target window. SLSHWCaptureSpace is the only capture primitive that
       returns opaque, accelerant-composited pixels on modern macOS. */
    CGDirectDisplayID did = CGMainDisplayID();
    CGRect fb = display_bounds(did);
    CGImageRef captured = SLSHWCaptureSpace(window_connection(),
                                            (int64_t)display_space_id(did),
                                            HW_CAPTURE_OPAQUE);
    if (captured) {
        size_t img_w = CGImageGetWidth(captured);
        size_t img_h = CGImageGetHeight(captured);
        double scale_x = (fb.size.width > 0.0) ? (double)img_w / fb.size.width : 1.0;
        double scale_y = (fb.size.height > 0.0) ? (double)img_h / fb.size.height : 1.0;

        /* CGWindowList bounds use a top-left origin in points; the space
           capture image is full-resolution with a top-left origin too. */
        CGRect crop = CGRectMake(bounds.origin.x * scale_x,
                                 bounds.origin.y * scale_y,
                                 bounds.size.width * scale_x,
                                 bounds.size.height * scale_y);

        crop = CGRectIntersection(crop,
                                  CGRectMake(0, 0, (CGFloat)img_w, (CGFloat)img_h));
        if (!CGRectIsEmpty(crop)) {
            CGImageRef cropped = CGImageCreateWithImageInRect(captured, crop);
            CFRelease(captured);
            return cropped;
        }
        CFRelease(captured);
    }

    /* Fallback: per-window capture through the window list. */
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

    CGImageRef captured = alias_capture_window(alias, wid, alias->window);
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