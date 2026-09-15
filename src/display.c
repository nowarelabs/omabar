#include "display.h"
#include "display_nsscreen.h"
#include "event.h"
#include "misc/extern.h"
#include "misc/helpers.h"
#include <CoreGraphics/CoreGraphics.h>

static CGDisplayReconfigurationCallBack g_callback = NULL;

#define DISPLAY_IDENTIFIER_KEY CFSTR("Display Identifier")
#define CURRENT_SPACE_KEY      CFSTR("Current Space")
#define DISPLAY_SPACES_KEY     CFSTR("Spaces")
#define SPACE_ID64_KEY         CFSTR("id64")
#define SPACE_ID_KEY           CFSTR("SpaceID")

#define MAX_DISPLAYS 8

static bool display_uuid_matches(CFStringRef uuid_string,
                                 CGDirectDisplayID did);
static uint64_t display_space_id_from_dict(CFDictionaryRef space_dict);

unsigned int display_arrangement_id(CGDirectDisplayID did) {
    if (!did) return 0;
    CFArrayRef displays = SLSCopyManagedDisplays(g_connection);
    if (!displays) return 0;

    CFIndex count = CFArrayGetCount(displays);
    for (CFIndex i = 0; i < count; i++) {
        const void *entry = CFArrayGetValueAtIndex(displays, i);
        if (CFGetTypeID(entry) == CFNumberGetTypeID()) {
            CGDirectDisplayID d = 0;
            CFNumberGetValue(entry, kCFNumberIntType, &d);
            if (d == did) {
                CFRelease(displays);
                return (unsigned int)i + 1;
            }
        } else if (CFGetTypeID(entry) == CFStringGetTypeID() &&
                   display_uuid_matches(entry, did)) {
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
    CFArrayRef managed = SLSCopyManagedDisplaySpaces(g_connection);
    uint64_t sid = 0;

    if (managed) {
        for (CFIndex i = 0; i < CFArrayGetCount(managed); i++) {
            CFDictionaryRef display_info = CFArrayGetValueAtIndex(managed, i);
            if (!display_info) continue;

            CFStringRef display_uuid = CFDictionaryGetValue(display_info,
                                                            DISPLAY_IDENTIFIER_KEY);
            if (!display_uuid || !display_uuid_matches(display_uuid, did))
                continue;

            CFDictionaryRef current = CFDictionaryGetValue(display_info,
                                                           CURRENT_SPACE_KEY);
            if (current && CFGetTypeID(current) == CFDictionaryGetTypeID())
                sid = display_space_id_from_dict(current);
            break;
        }
        CFRelease(managed);
    }

    if (!sid) sid = SLSGetActiveSpace(g_connection);
    return sid;
}

uint64_t display_space_display_id(uint64_t sid) {
    CGDirectDisplayID result = 0;
    if (!sid) return result;

    CFStringRef display_uuid = SLSCopyManagedDisplayForSpace(g_connection, sid);
    if (!display_uuid) return result;

    uint32_t count = 0;
    CGDirectDisplayID displays[MAX_DISPLAYS];
    if (CGGetActiveDisplayList(MAX_DISPLAYS, displays, &count) != kCGErrorSuccess) {
        CFRelease(display_uuid);
        return result;
    }

    CFUUIDRef wanted = CFUUIDCreateFromString(kCFAllocatorDefault, display_uuid);
    for (uint32_t i = 0; i < count && wanted; i++) {
        CFUUIDRef candidate = CGDisplayCreateUUIDFromDisplayID(displays[i]);
        if (candidate && CFEqual(wanted, candidate))
            result = displays[i];
        if (candidate) CFRelease(candidate);
        if (result) break;
    }
    if (wanted) CFRelease(wanted);
    CFRelease(display_uuid);
    return result;
}

static bool display_uuid_matches(CFStringRef uuid_string,
                                 CGDirectDisplayID did) {
    CFUUIDRef display_uuid = CGDisplayCreateUUIDFromDisplayID(did);
    if (!display_uuid) return false;

    CFUUIDRef uuid = CFUUIDCreateFromString(kCFAllocatorDefault, uuid_string);
    bool matches = uuid ? (CFEqual(display_uuid, uuid)) : false;
    if (uuid) CFRelease(uuid);
    CFRelease(display_uuid);
    return matches;
}

static uint64_t display_space_id_from_dict(CFDictionaryRef space_dict) {
    uint64_t sid = 0;

    CFNumberRef id64 = CFDictionaryGetValue(space_dict, SPACE_ID64_KEY);
    if (id64 && CFGetTypeID(id64) == CFNumberGetTypeID()) {
        CFNumberGetValue(id64, kCFNumberSInt64Type, &sid);
        return sid;
    }

    /* legacy macOS builds use "SpaceID" instead of "id64" */
    CFNumberRef legacy = CFDictionaryGetValue(space_dict, SPACE_ID_KEY);
    if (legacy && CFGetTypeID(legacy) == CFNumberGetTypeID())
        CFNumberGetValue(legacy, kCFNumberSInt64Type, &sid);

    return sid;
}

uint64_t *display_space_list(CGDirectDisplayID did, int *space_count) {
    *space_count = 0;

    CFArrayRef managed = SLSCopyManagedDisplaySpaces(g_connection);
    if (!managed) return NULL;

    uint64_t *result = NULL;
    int count = 0;

    for (CFIndex i = 0; i < CFArrayGetCount(managed); i++) {
        CFDictionaryRef display_info = CFArrayGetValueAtIndex(managed, i);
        if (!display_info) continue;

        CFStringRef display_uuid = CFDictionaryGetValue(display_info,
                                                        DISPLAY_IDENTIFIER_KEY);
        if (!display_uuid || !display_uuid_matches(display_uuid, did)) continue;

        CFArrayRef spaces = CFDictionaryGetValue(display_info,
                                                 DISPLAY_SPACES_KEY);
        if (!spaces) continue;

        for (CFIndex j = 0; j < CFArrayGetCount(spaces); j++) {
            CFDictionaryRef space_info = CFArrayGetValueAtIndex(spaces, j);
            uint64_t sid = space_info ? display_space_id_from_dict(space_info) : 0;
            if (!sid) continue;

            buf_push(result, sid);
            count++;
        }
        break;
    }

    CFRelease(managed);
    *space_count = count;
    return result;
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
    CGRect safe = display_nsscreen_safe_area(did);
    if (CGRectIsNull(safe)) return display_bounds(did);
    return safe;
}

void display_observe(int *did) {
    (void)did;
    display_begin();
}

void display_brightness_begin(void) {
}

void display_brightness_end(void) {
}