#include "display_nsscreen.h"
#include <AppKit/AppKit.h>

CGRect display_nsscreen_safe_area(CGDirectDisplayID did) {
    NSScreen *screen = [NSScreen screenWithDisplayID:did];
    if (!screen) return CGRectNull;

    NSEdgeInsets insets = [screen safeAreaInsets];
    return CGRectMake(insets.left, insets.bottom, insets.right, insets.top);
}