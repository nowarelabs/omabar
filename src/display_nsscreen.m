#include "display_nsscreen.h"
#include <AppKit/AppKit.h>

static NSScreen *screen_for_display(CGDirectDisplayID did) {
    for (NSScreen *screen in NSScreen.screens) {
        NSNumber *screen_number =
            [[screen deviceDescription] objectForKey:@"NSScreenNumber"];
        if (screen_number && [screen_number unsignedIntValue] == did)
            return screen;
    }
    return nil;
}

CGRect display_nsscreen_safe_area(CGDirectDisplayID did) {
    NSScreen *screen = screen_for_display(did);
    if (!screen) return CGRectNull;

    NSEdgeInsets insets = [screen safeAreaInsets];
    return CGRectMake(insets.left, insets.bottom, insets.right, insets.top);
}

double display_nsscreen_top_inset(CGDirectDisplayID did) {
    NSScreen *screen = screen_for_display(did);
    if (!screen) return 0.0;

    NSEdgeInsets insets = [screen safeAreaInsets];
    return insets.top;
}