#include "workspace.h"
#include "event.h"
#include "display.h"
#include <AppKit/AppKit.h>
#include <objc/runtime.h>

static NSObject *g_workspace_observer = NULL;

@interface OmabarWorkspaceObserver : NSObject
- (void)workspaceEventHandler:(NSNotification *)note;
- (void)applicationDidWakeHandler:(NSNotification *)note;
- (void)activeDisplayDidChangeHandler:(NSNotification *)note;
- (void)menuBarHiddenDidChangeHandler:(NSNotification *)note;
@end

@implementation OmabarWorkspaceObserver

- (void)workspaceEventHandler:(NSNotification *)note {
    (void)note;
    struct event event = { .type = EVENT_SPACE_CHANGED };
    event_post(&event);
}

- (void)applicationDidWakeHandler:(NSNotification *)note {
    (void)note;
}

- (void)activeDisplayDidChangeHandler:(NSNotification *)note {
    (void)note;
    struct event event = { .type = EVENT_DISPLAY_MOVED };
    event_post(&event);
}

- (void)menuBarHiddenDidChangeHandler:(NSNotification *)note {
    (void)note;
}

@end

void workspace_event_handler_init(void) {
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
}

void workspace_event_handler_begin(void) {
}

void workspace_event_handler_end(void) {
    if (!g_workspace_observer) return;
    NSNotificationCenter *nc = [[NSWorkspace sharedWorkspace] notificationCenter];
    [nc removeObserver:g_workspace_observer];
    g_workspace_observer = NULL;
}