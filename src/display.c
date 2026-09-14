#include "display.h"
#include "event.h"
#include "misc/extern.h"
#include "misc/helpers.h"
#include <CoreGraphics/CoreGraphics.h>

static CGDisplayReconfigurationCallBack g_callback = NULL;

unsigned int display_arrangement_id(CGDirectDisplayID did) {
    if (!did) return 0;
    CFArrayRef displays = SLSCopyManagedDisplays(g_connection);
    if (!displays) return 0;

    CFIndex count = CFArrayGetCount(displays);
    for (CFIndex i = 0; i < count; i++) {
        const CFNumberRef num = CFArrayGetValueAtIndex(displays, i);
        CGDirectDisplayID d = 0;
        CFNumberGetValue(num, kCFNumberIntType, &d);
        if (d == did) {
            CFRelease(displays);
            return (unsigned int)i + 1;
        }
    }
    CFRelease(displays);
    return 0;
}

CGRect display_bounds(CGDirectDisplayID did) {
    return CGDisplayBounds(did);
}

uint64_t display_space_id(CGDirectDisplayID did) {
    CFStringRef uuid = SLSManagedDisplayGetCurrentSpace(g_connection, did);
    if (!uuid) return 0;

    uint64_t sid = 0;
    CFNumberRef num = SLSGetSpaceIDForUUID(g_connection, uuid);
    if (num) {
        CFNumberGetValue(num, kCFNumberSInt64Type, &sid);
        CFRelease(num);
    }
    CFRelease(uuid);
    return sid;
}

uint64_t display_space_display_id(uint64_t sid) {
    return SLSGetDisplayIDForSpace(g_connection, sid);
}

static void display_reconfiguration_callback(CGDirectDisplayID display,
                                             CGDisplayChangeSummaryFlags flags,
                                             void *user_info) {
    (void)user_info;
    struct event event = {0};

    if (flags & kCGDisplayAddFlag) {
        event.type = EVENT_DISPLAY_ADDED;
    } else if (flags & kCGDisplayRemoveFlag) {
        event.type = EVENT_DISPLAY_REMOVED;
    } else {
        event.type = EVENT_DISPLAY_MOVED;
    }
    event.arg1 = (uint64_t)display;
    event_post(&event);
}

int display_begin(void) {
    CGDisplayRegisterReconfigurationCallback(display_reconfiguration_callback, NULL);
    return 0;
}

void display_end(void) {
    CGDisplayRemoveReconfigurationCallback(display_reconfiguration_callback, NULL);
}

bool display_has_notch(CGDirectDisplayID did) {
    CGRect bounds = CGDisplayBounds(did);
    return (int)bounds.size.width == 1512 || (int)bounds.size.width == 1728 ||
           (int)bounds.size.width == 2560 || (int)bounds.size.width == 3024;
}

CGRect display_safe_area(CGDirectDisplayID did) {
    return display_bounds(did);
}

void display_observe(int *did) {
    (void)did;
}

void display_brightness_begin(void) {
}

void display_brightness_end(void) {
}