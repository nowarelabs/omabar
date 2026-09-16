#include "workspace.h"
#include "event.h"
#include "display.h"
#include "dnd.h"
#include "misc/helpers.h"
#include "misc/extern.h"
#include "window.h"
#include <AppKit/AppKit.h>
#include <objc/runtime.h>

#ifndef MAX_DISPLAYS
#define MAX_DISPLAYS 8
#endif

static NSObject *g_workspace_observer = NULL;

/*
 * activeDisplayDidChangeNotification and
 * menuBarDidChangeVisibilityNotification are Apple-private events that
 * are not exposed by the public SDK, so they must be registered by name
 * (Sketchybar does the same).
 */
static NSString *kActiveDisplayDidChangeNotification =
    @"activeDisplayDidChangeNotification";
static NSString *kMenuBarDidChangeVisibilityNotification =
    @"menuBarDidChangeVisibilityNotification";

/* Per-display last-known space id, keyed by display id (helpers.h table). */
static struct table g_space_log;

struct space_log_entry {
    uint32_t did;
    uint64_t sid;
};

static unsigned long space_log_hash(void *key) {
    return (unsigned long)*(uint32_t *)key;
}

static int space_log_compare(void *key1, void *key2) {
    return *(uint32_t *)key1 == *(uint32_t *)key2;
}

static void space_log_init(void) {
    if (!g_space_log.buckets)
        table_init(&g_space_log, 16, space_log_hash, space_log_compare);
}

/*
 * Resolve the space change that just happened: scan active displays,
 * compare each display's current space id against the last-known value
 * stored in the log, and post EVENT_SPACE_CHANGED carrying
 *      arg1 = new space id
 *      arg2 = old space id
 *      data = display id (as integer cast, previously zero => first run)
 * Returns true if a change was detected on some display.
 */
static bool post_space_changed_event(void) {
    uint32_t displays[MAX_DISPLAYS];
    uint32_t count = 0;
    if (CGGetActiveDisplayList((uint32_t)MAX_DISPLAYS, displays, &count)
        != kCGErrorSuccess || count == 0)
        return false;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t did = displays[i];
        uint64_t new_sid = display_space_id(did);

        struct space_log_entry *entry =
            table_find(&g_space_log, &did);

        if (entry && entry->sid != new_sid) {
            struct event event = {
                .type = EVENT_SPACE_CHANGED,
                .arg1 = new_sid,
                .arg2 = entry->sid,
                .data = (void *)(intptr_t)did,
            };
            entry->sid = new_sid;
            event_post(&event);
            return true;
        }

        /* first observation: seed the log without posting */
        if (!entry) {
            struct space_log_entry *rec = malloc(sizeof(*rec));
            rec->did = did;
            rec->sid = new_sid;
            table_add(&g_space_log, &did, rec);
        }
    }
    return false;
}

typedef struct {
    const char *title;
    uint64_t old_sid;
    uint64_t new_sid;
    uint32_t did;
} space_change_snapshot;

static void space_event_handler(uint32_t event, void *data,
                                size_t data_length, void *context) {
    (void)event; (void)data; (void)data_length; (void)context;
    space_log_init();
    post_space_changed_event();
}

/* Re-scan every display and post events for each changed space. */
void workspace_recheck_space_change(void) {
    space_log_init();
    post_space_changed_event();
}

void forced_space_change_event(void) {
    space_log_init();
    post_space_changed_event();
}

@interface OmabarWorkspaceObserver : NSObject
- (void)workspaceEventHandler:(NSNotification *)note;
- (void)applicationDidWakeHandler:(NSNotification *)note;
- (void)activeDisplayDidChangeHandler:(NSNotification *)note;
- (void)menuBarHiddenDidChangeHandler:(NSNotification *)note;
@end

@implementation OmabarWorkspaceObserver

- (void)workspaceEventHandler:(NSNotification *)note {
    (void)note;
    space_log_init();
    post_space_changed_event();
}

- (void)applicationDidWakeHandler:(NSNotification *)note {
    (void)note;
    dnd_update();
    struct event event = { .type = EVENT_POWER_CHANGED };
    event_post(&event);
}

- (void)activeDisplayDidChangeHandler:(NSNotification *)note {
    (void)note;
    struct event event = { .type = EVENT_DISPLAY_MOVED };
    event_post(&event);
}

- (void)menuBarHiddenDidChangeHandler:(NSNotification *)note {
    (void)note;
    struct event event = { .type = EVENT_MENU_BAR_HIDDEN_CHANGED };
    event_post(&event);
}

@end

void workspace_event_handler_init(void) {
    if (g_workspace_observer) return;

    NSWorkspace *ws = [NSWorkspace sharedWorkspace];
    NSNotificationCenter *nc = [ws notificationCenter];
    g_workspace_observer = [[OmabarWorkspaceObserver alloc] init];

    [nc addObserver:g_workspace_observer
           selector:@selector(workspaceEventHandler:)
               name:NSWorkspaceActiveSpaceDidChangeNotification
             object:nil];

    [nc addObserver:g_workspace_observer
           selector:@selector(applicationDidWakeHandler:)
               name:NSWorkspaceDidWakeNotification
             object:nil];

    [nc addObserver:g_workspace_observer
           selector:@selector(activeDisplayDidChangeHandler:)
               name:kActiveDisplayDidChangeNotification
             object:nil];

    [nc addObserver:g_workspace_observer
           selector:@selector(menuBarHiddenDidChangeHandler:)
               name:kMenuBarDidChangeVisibilityNotification
             object:nil];
}

void workspace_event_handler_begin(void) {
    space_log_init();
    workspace_event_handler_init();

    uint32_t connection = window_connection();

    /* SkyLight space-change notification procs. 1401 is the classic space
       change event; 1327/1328 are the macOS 13+ space change events. */
    SLSRegisterNotifyProc((void *)space_event_handler, 1401, NULL);
    if (__builtin_available(macOS 13.0, *)) {
        SLSRegisterNotifyProc((void *)space_event_handler, 1327, NULL);
        SLSRegisterNotifyProc((void *)space_event_handler, 1328, NULL);
    }
    (void)connection;

    /* seed the space log with the current space on every display */
    post_space_changed_event();
}

void workspace_event_handler_end(void) {
    if (!g_workspace_observer) return;
    NSNotificationCenter *nc = [[NSWorkspace sharedWorkspace] notificationCenter];
    [nc removeObserver:g_workspace_observer];
    g_workspace_observer = NULL;

    if (g_space_log.buckets) {
        for (int i = 0; i < g_space_log.capacity; i++) {
            struct bucket *bucket = g_space_log.buckets[i];
            while (bucket) {
                struct space_log_entry *rec = bucket->value;
                free(rec);
                bucket = bucket->next;
            }
        }
        table_free(&g_space_log);
    }
}