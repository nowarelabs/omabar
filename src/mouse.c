#include "mouse.h"
#include "misc/helpers.h"
#include <Carbon/Carbon.h>

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

    GetEventParameter(event, kEventParamMouseLocation, typeQDPoint,
                      NULL, sizeof(Point), NULL, NULL);

    switch (event_kind) {
        case kEventMouseDown:
            me.type = 1;
            GetEventParameter(event, kEventParamClickCount, typeSInt32,
                              &type, sizeof(value), NULL, &value);
            me.button = (uint32_t)value;
            break;
        case kEventMouseUp:
            me.type = 2;
            GetEventParameter(event, kEventParamClickCount, typeSInt32,
                              &type, sizeof(value), NULL, &value);
            me.button = (uint32_t)value;
            break;
        case kEventMouseMoved:
        case kEventMouseDragged:
            me.type = 3;
            break;
        case kEventMouseWheelMoved:
            me.type = 5;
            GetEventParameter(event, kEventParamMouseWheelDelta, typeSInt32,
                              &type, sizeof(value), NULL, &value);
            me.scroll_delta = (int)value;
            break;
    }

    GetEventParameter(event, kEventParamKeyModifiers, typeUInt32,
                      &type, sizeof(me.modifier), NULL, &me.modifier);

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