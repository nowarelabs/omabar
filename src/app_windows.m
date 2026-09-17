#include "app_windows.h"
#include "event.h"
#include "misc/helpers.h"
#include <Carbon/Carbon.h>
#include <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef OSStatus (*PGetFrontProcess)(ProcessSerialNumber *psn);
typedef OSStatus (*PGetProcessForPID)(pid_t pid, ProcessSerialNumber *psn);
typedef OSStatus (*PGetProcessName)(ProcessSerialNumber *psn, CFStringRef *name);

struct app_record {
    pid_t pid;
    pid_t fwid;
};

static struct table g_app_registry;

static unsigned long app_hash(void *key) {
    return (unsigned long) *(pid_t *)key;
}

static int app_compare(void *a, void *b) {
    return *(pid_t *)a == *(pid_t *)b;
}

static struct app_record *app_record_for_pid(pid_t pid) {
    struct app_record *rec = table_find(&g_app_registry, &pid);
    if (!rec) {
        rec = calloc(1, sizeof(*rec));
        if (!rec) return NULL;
        rec->pid = pid;
        table_add(&g_app_registry, &pid, rec);
    }
    return rec;
}

static pid_t app_front_pid(void) {
    NSWorkspace *ws = [NSWorkspace sharedWorkspace];
    NSRunningApplication *front = ws.frontmostApplication;
    return front ? (pid_t)front.processIdentifier : 0;
}

static void ax_title_changed(AXObserverRef observer, AXUIElementRef element,
                             CFStringRef notification, void *context) {
    (void)observer; (void)element; (void)notification; (void)context;

    pid_t pid;
    if (AXUIElementGetPid(element, &pid) != kAXErrorSuccess) return;

    struct app_record *rec = app_record_for_pid(pid);
    if (!rec) return;

    rec->fwid = 0;
    struct event event = {
        .type = EVENT_WINDOW_FOCUSED,
        .arg1 = (uint64_t)pid,
        .arg2 = 0
    };
    event_post(&event);
}

static AXObserverRef observer_create_for_app(pid_t pid) {
    AXObserverRef observer = NULL;
    AXObserverCreate(pid, ax_title_changed, &observer);
    return observer;
}

static void register_ax_observer(pid_t pid) {
    AXUIElementRef app = AXUIElementCreateApplication(pid);
    if (!app) return;

    AXObserverRef observer = observer_create_for_app(pid);
    if (!observer) {
        CFRelease(app);
        return;
    }

    AXObserverRemoveNotification(observer, app, kAXFocusedWindowChangedNotification);
    AXObserverRemoveNotification(observer, app, kAXTitleChangedNotification);

    AXObserverAddNotification(observer, app, kAXFocusedWindowChangedNotification, (void *)app);
    AXObserverAddNotification(observer, app, kAXTitleChangedNotification, (void *)app);

    CFRunLoopSourceRef source = AXObserverGetRunLoopSource(observer);
    if (source)
        CFRunLoopAddSource(CFRunLoopGetMain(), source,
                           kCFRunLoopCommonModes);

    CFRelease(observer);
    CFRelease(app);
}

static void unregister_ax_observer(pid_t pid) {
    struct app_record *rec = table_find(&g_app_registry, &pid);
    if (!rec) return;
    table_remove(&g_app_registry, &pid);
    free(rec);
}

static pascal OSStatus app_launched(pid_t pid) {
    struct event event = {
        .type = EVENT_APP_LAUNCHED,
        .arg1 = (uint64_t)pid
    };
    event_post(&event);
    return noErr;
}

static pascal OSStatus app_terminated(pid_t pid) {
    unregister_ax_observer(pid);
    struct event event = {
        .type = EVENT_APP_TERMINATED,
        .arg1 = (uint64_t)pid
    };
    event_post(&event);
    return noErr;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
static void app_front_switched(ProcessSerialNumber psn) {
    CFStringRef name = NULL;
    OSStatus err = CopyProcessName(&psn, &name);
    char buffer[256] = {0};
    if (err == noErr) {
        CFStringGetCString(name, buffer, sizeof(buffer),
                           kCFStringEncodingUTF8);
        CFRelease(name);
    }

    pid_t pid = 0;
    GetProcessPID(&psn, &pid);

    register_ax_observer(pid);

    struct event event = {
        .type = EVENT_APP_FRONT_SWITCHED,
        .arg1 = (uint64_t)pid
    };
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

    pid_t pid = 0;
    GetProcessPID(&psn, &pid);

    switch (kind) {
        case kEventAppLaunched:
            return app_launched(pid);
        case kEventAppTerminated:
            return app_terminated(pid);
        case kEventAppFrontSwitched:
            app_front_switched(psn);
            return noErr;
    }

    return noErr;
}
#pragma clang diagnostic pop

static EventHandlerUPP g_process_handler_ref;

void app_windows_begin(void) {
    table_init(&g_app_registry, 32, app_hash, app_compare);

    EventTypeSpec events[] = {
        { kEventClassApplication, kEventAppLaunched },
        { kEventClassApplication, kEventAppTerminated },
        { kEventClassApplication, kEventAppFrontSwitched }
    };

    g_process_handler_ref = NewEventHandlerUPP(process_handler);
    InstallEventHandler(GetEventDispatcherTarget(),
                        g_process_handler_ref,
                        3, events, NULL, NULL);

    pid_t front = app_front_pid();
    if (front) register_ax_observer(front);
}

void app_windows_end(void) {
    if (g_process_handler_ref)
        DisposeEventHandlerUPP(g_process_handler_ref);
    table_free(&g_app_registry);
}

const char *app_windows_front_app_name(void) {
    NSWorkspace *ws = [NSWorkspace sharedWorkspace];
    NSRunningApplication *front = ws.frontmostApplication;
    if (!front) return NULL;
    return [front.localizedName UTF8String];
}

int app_windows_is_observing(const char *pid) {
    if (!pid) return 0;

    pid_t value = (pid_t)strtol(pid, NULL, 10);
    return table_find(&g_app_registry, &value) != NULL;
}

pid_t app_windows_pid_for_bundle_id(const char *bundle_id) {
    if (!bundle_id || !*bundle_id) return 0;

    NSString *identifier =
        [NSString stringWithUTF8String:bundle_id];
    NSArray<NSRunningApplication *> *apps =
        [NSRunningApplication runningApplicationsWithBundleIdentifier:identifier];
    for (NSRunningApplication *app in apps) {
        if (app.activationPolicy == NSApplicationActivationPolicyRegular)
            return (pid_t)app.processIdentifier;
    }
    return apps.count > 0 ? (pid_t)apps.firstObject.processIdentifier : 0;
}

const char *app_windows_name_for_pid(pid_t pid, char *buf, size_t len) {
    if (!buf || len == 0) return NULL;
    buf[0] = '\0';

    NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
    NSString *name = app ? app.localizedName : nil;
    if (!name) return NULL;

    const char *utf8 = name.UTF8String;
    snprintf(buf, len, "%s", utf8 ? utf8 : "");
    return buf;
}