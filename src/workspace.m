#include "workspace.h"
#include "event.h"
#include <AppKit/AppKit.h>
#include <objc/runtime.h>

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
    workspace_event_handler_init();
}

void workspace_event_handler_end(void) {
    if (!g_workspace_observer) return;
    NSNotificationCenter *nc = [[NSWorkspace sharedWorkspace] notificationCenter];
    [nc removeObserver:g_workspace_observer];
    g_workspace_observer = NULL;
}