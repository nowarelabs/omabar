#include "dnd.h"
#include "event.h"
#include "misc/helpers.h"
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool g_dnd_state = false;
static char *g_dnd_path = NULL;

static void dnd_load_path(void) {
    if (g_dnd_path) return;

    const char *home = getenv("HOME");
    if (!home) return;

    const char *base = "/Library/Preferences/com.apple.notificationcenterui.plist";
    size_t len = strlen(home) + strlen(base) + 1;
    g_dnd_path = malloc(len);
    if (!g_dnd_path) return;
    snprintf(g_dnd_path, len, "%s%s", home, base);
}

void dnd_init(void) {
    dnd_load_path();
}

bool dnd_get_state(void) {
    return g_dnd_state;
}

/*
 * Re-read the com.apple.notificationcenterui preferences plist and update
 * the cached DND state. Posts EVENT_CUSTOM ("dnd" / "dnd_end") when the
 * state flips. Sleep / wake is handled by the workspace observer calling
 * dnd_update() after wake.
 */
void dnd_update(void) {
    dnd_load_path();
    if (!g_dnd_path) return;

    CFDataRef plist = NULL;
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
        NULL, (const UInt8 *)g_dnd_path, strlen(g_dnd_path), false);
    if (!url) return;

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    if (CFURLCreateDataAndPropertiesFromResource(
            NULL, url, &plist, NULL, NULL, NULL) != 0)
        plist = NULL;
    #pragma clang diagnostic pop
    CFRelease(url);

    if (!plist) return;

    CFPropertyListRef props = CFPropertyListCreateWithData(
        NULL, plist, kCFPropertyListImmutable, NULL, NULL);
    CFRelease(plist);

    if (!props) return;

    bool plist_state = false;
    CFBooleanRef dnd = NULL;
    if (CFGetTypeID(props) == CFDictionaryGetTypeID()) {
        CFDictionaryRef dict = (CFDictionaryRef)props;
        dnd = CFDictionaryGetValue(dict, CFSTR("doNotDisturb"));
        if (dnd && CFGetTypeID(dnd) == CFBooleanGetTypeID())
            plist_state = CFBooleanGetValue(dnd);
    }
    CFRelease(props);

    if (plist_state == g_dnd_state) return;

    g_dnd_state = plist_state;

    struct event event = {
        .type = EVENT_CUSTOM,
        .arg1 = (uint64_t)g_dnd_state,
    };
    event.data = string_copy(g_dnd_state ? "dnd" : "dnd_end");
    event_post(&event);
}