#include "mouse.h"
#include "misc/helpers.h"
#include <Carbon/Carbon.h>

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
static EventHandlerRef g_event_handler_ref = NULL;

static OSStatus mouse_callback(EventHandlerCallRef call_ref, EventRef event, void *data) {
    (void)call_ref; (void)data;
    if (!g_handler) return noErr;

    UInt32 event_kind = GetEventKind(event);
    EventParamType type;
    SInt32 value;

    struct mouse_event me;
    memset(&me, 0, sizeof(me));

    CGEventRef cg_event = CopyEventCGEvent(event);
    if (cg_event) {
        me.location = CGEventGetLocation(cg_event);
        me.modifier = (uint32_t)CGEventGetFlags(cg_event);
        me.button = (uint32_t)CGEventGetIntegerValueField(cg_event,
                                                          kCGMouseEventButtonNumber);
        CFRelease(cg_event);
    } else {
        Point qd = { 0, 0 };
        if (GetEventParameter(event, kEventParamMouseLocation, typeQDPoint,
                              NULL, sizeof(Point), NULL, &qd) == noErr) {
            me.location = CGPointMake(qd.h, qd.v);
        }
    }

    switch (event_kind) {
        case kEventMouseDown:
            me.type = MOUSE_EVENT_DOWN;
            break;
        case kEventMouseUp:
            me.type = MOUSE_EVENT_UP;
            break;
        case kEventMouseMoved:
            me.type = MOUSE_EVENT_MOVED;
            break;
        case kEventMouseDragged:
            me.type = MOUSE_EVENT_DRAGGED;
            break;
        case kEventMouseWheelMoved:
            me.type = MOUSE_EVENT_SCROLLED;
            GetEventParameter(event, kEventParamMouseWheelDelta, typeSInt32,
                              &type, sizeof(value), NULL, &value);
            me.scroll_delta = (int)value;
            break;
    }

    g_handler(&me);
    return noErr;
}

void mouse_begin(mouse_handler_fn handler) {
    if (!handler) return;
    g_handler = handler;

    EventTypeSpec events[] = {
        { kEventClassMouse, kEventMouseDown },
        { kEventClassMouse, kEventMouseUp },
        { kEventClassMouse, kEventMouseMoved },
        { kEventClassMouse, kEventMouseDragged },
        { kEventClassMouse, kEventMouseWheelMoved }
    };
    int count = sizeof(events) / sizeof(events[0]);
    InstallEventHandler(GetEventDispatcherTarget(), mouse_callback,
                        count, events, NULL, &g_event_handler_ref);
}

void mouse_end(void) {
    if (g_event_handler_ref) {
        RemoveEventHandler(g_event_handler_ref);
        g_event_handler_ref = NULL;
    }
    g_handler = NULL;
}

void mouse_set_window(uint32_t window_id) {
    g_window = window_id;
}