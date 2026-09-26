#include "mouse.h"
#include "misc/helpers.h"
#include <ApplicationServices/ApplicationServices.h>
#include <syslog.h>
#include <unistd.h>

/* types reported to the handler (kept as small ints to stay self-contained) */
enum {
    MOUSE_EVENT_DOWN = 1,
    MOUSE_EVENT_UP = 2,
    MOUSE_EVENT_MOVED = 3,
    MOUSE_EVENT_DRAGGED = 4,
    MOUSE_EVENT_SCROLLED = 5
};

static mouse_handler_fn g_handler = NULL;
static uint32_t g_window = 0;
static CFMachPortRef g_tap = NULL;
static CFRunLoopSourceRef g_tap_source = NULL;

/* The previous implementation installed a Carbon handler on
   GetEventDispatcherTarget(), which only receives events while an
   NSApplication pumps its event loop. omabar has no NSApplication — it
   runs CFRunLoopRun() — so that target never fired and nothing was ever
   clickable. A CGEventTap bound to the main run loop is the equivalent
   for this kind of app. */
static CGEventRef mouse_tap_callback(CGEventTapProxy proxy, CGEventType type,
                                     CGEventRef event, void *data) {
    (void)proxy;
    (void)data;

    if (type == kCGEventTapDisabledByTimeout
        || type == kCGEventTapDisabledByUserInput) {
        if (g_tap) CGEventTapEnable(g_tap, true);
        return event;
    }

    if (!g_handler || !event) return event;

    struct mouse_event me;
    memset(&me, 0, sizeof(me));
    /* Quartz global space: origin top-left of the primary display, matching
       the window origins the hit tests compare against. */
    me.location = CGEventGetLocation(event);
    me.modifier = (uint32_t)CGEventGetFlags(event);
    me.button = (uint32_t)CGEventGetIntegerValueField(event,
                                                      kCGMouseEventButtonNumber);

    switch (type) {
        case kCGEventLeftMouseDown:
        case kCGEventRightMouseDown:
        case kCGEventOtherMouseDown:
            me.type = MOUSE_EVENT_DOWN;
            break;
        case kCGEventLeftMouseUp:
        case kCGEventRightMouseUp:
        case kCGEventOtherMouseUp:
            me.type = MOUSE_EVENT_UP;
            break;
        case kCGEventMouseMoved:
            me.type = MOUSE_EVENT_MOVED;
            break;
        case kCGEventLeftMouseDragged:
        case kCGEventRightMouseDragged:
        case kCGEventOtherMouseDragged:
            me.type = MOUSE_EVENT_DRAGGED;
            break;
        case kCGEventScrollWheel:
            me.type = MOUSE_EVENT_SCROLLED;
            me.scroll_delta = (int)CGEventGetIntegerValueField(
                event, kCGScrollWheelEventDeltaAxis1);
            break;
        default:
            return event;
    }

    g_handler(&me);
    return event;
}

static void mouse_notify_unclickable(void) {
    /* Best effort: fork off so a missing osascript can never block startup. */
    pid_t pid = fork();
    if (pid != 0) return;
    alarm(5);
    execlp("osascript", "osascript", "-e",
           "display notification \"Clicks need Accessibility permission. "
           "Grant it in System Settings > Privacy & Security > "
           "Accessibility.\" with title \"Omabar\"",
           (char *)NULL);
    _exit(127);
}

void mouse_begin(mouse_handler_fn handler) {
    if (!handler) return;
    g_handler = handler;
    if (g_tap) return;

    CGEventMask mask = CGEventMaskBit(kCGEventLeftMouseDown)
                     | CGEventMaskBit(kCGEventRightMouseDown)
                     | CGEventMaskBit(kCGEventOtherMouseDown)
                     | CGEventMaskBit(kCGEventLeftMouseUp)
                     | CGEventMaskBit(kCGEventRightMouseUp)
                     | CGEventMaskBit(kCGEventOtherMouseUp)
                     | CGEventMaskBit(kCGEventMouseMoved)
                     | CGEventMaskBit(kCGEventLeftMouseDragged)
                     | CGEventMaskBit(kCGEventRightMouseDragged)
                     | CGEventMaskBit(kCGEventOtherMouseDragged)
                     | CGEventMaskBit(kCGEventScrollWheel);

    g_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                             kCGEventTapOptionDefault, mask,
                             mouse_tap_callback, NULL);
    if (!g_tap) {
        /* Almost always a missing Accessibility/Input Monitoring grant:
           CGEventTapCreate returns NULL without it. The symptom is otherwise
           just "the bar is not clickable" with no explanation anywhere the
           user is likely to look, so also raise a notification. */
        const char *msg = "omabar: CGEventTapCreate failed - grant the "
                          "omabar binary Accessibility (or Input "
                          "Monitoring) permission in System Settings > "
                          "Privacy & Security, or bar items stay "
                          "unclickable";
        fprintf(stderr, "%s\n", msg);
        syslog(LOG_ERR, "%s", msg);
        mouse_notify_unclickable();
        return;
    }

    g_tap_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, g_tap, 0);
    if (!g_tap_source) {
        CFRelease(g_tap);
        g_tap = NULL;
        return;
    }
    CFRunLoopAddSource(CFRunLoopGetMain(), g_tap_source, kCFRunLoopCommonModes);
    CGEventTapEnable(g_tap, true);
}

void mouse_end(void) {
    if (g_tap_source) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), g_tap_source,
                              kCFRunLoopCommonModes);
        CFRelease(g_tap_source);
        g_tap_source = NULL;
    }
    if (g_tap) {
        CGEventTapEnable(g_tap, false);
        CFRelease(g_tap);
        g_tap = NULL;
    }
    g_handler = NULL;
}

void mouse_set_window(uint32_t window_id) {
    g_window = window_id;
}
