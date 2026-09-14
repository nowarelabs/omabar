#include "app_windows.h"
#include "event.h"
#include <Carbon/Carbon.h>
#include <AppKit/AppKit.h>
#include <stdio.h>

typedef OSStatus (*PGetFrontProcess)(ProcessSerialNumber *psn);
typedef OSStatus (*PGetProcessForPID)(pid_t pid, ProcessSerialNumber *psn);
typedef OSStatus (*PGetProcessName)(ProcessSerialNumber *psn, CFStringRef *name);

static void app_front_switched(ProcessSerialNumber psn) {
    CFStringRef name = NULL;
    OSStatus err = CopyProcessName(&psn, &name);
    if (err != noErr) return;

    char buffer[256];
    CFStringGetCString(name, buffer, sizeof(buffer), kCFStringEncodingUTF8);
    CFRelease(name);

    struct event event = { .type = EVENT_APP_FRONT_SWITCHED };
    event.data = strdup(buffer);
    event_post(&event);
}

static pascal OSStatus process_handler(EventHandlerCallRef call,
                                       EventRef event, void *data) {
    (void)call; (void)data;
    UInt32 kind = GetEventKind(event);

    ProcessSerialNumber psn;
    OSStatus err = GetEventParameter(event, kEventParamProcessID,
                                     typeProcessSerialNumber, NULL,
                                     sizeof(psn), NULL, &psn);
    if (err != noErr) return err;

    struct event ev = {0};

    switch (kind) {
        case kEventAppLaunched:
            ev.type = EVENT_APP_LAUNCHED;
            break;
        case kEventAppTerminated:
            ev.type = EVENT_APP_TERMINATED;
            break;
        case kEventAppFrontSwitched:
            app_front_switched(psn);
            return noErr;
    }

    ev.arg1 = (uint64_t)psn.highLongOfPSN;
    event_post(&ev);
    return noErr;
}

static EventHandlerUPP g_process_handler_ref;

void app_windows_begin(void) {
    EventTypeSpec events[] = {
        { kEventClassApplication, kEventAppLaunched },
        { kEventClassApplication, kEventAppTerminated },
        { kEventClassApplication, kEventAppFrontSwitched }
    };

    g_process_handler_ref = NewEventHandlerUPP(process_handler);
    InstallEventHandler(GetEventDispatcherTarget(),
                        g_process_handler_ref,
                        3, events, NULL, NULL);
}

void app_windows_end(void) {
    if (g_process_handler_ref)
        DisposeEventHandlerUPP(g_process_handler_ref);
}

const char *app_windows_front_app_name(void) {
    NSWorkspace *ws = [NSWorkspace sharedWorkspace];
    NSRunningApplication *front = ws.frontmostApplication;
    if (!front) return NULL;
    return [front.localizedName UTF8String];
}

int app_windows_is_observing(const char *pid) {
    (void)pid;
    return 0;
}