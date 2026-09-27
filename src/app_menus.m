#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include "app_menus.h"

#include <stdlib.h>
#include <string.h>

/* A busy or wedged app must not stall the bar, and every read here is a
   synchronous round trip into someone else's process, so each one is capped.
   A hung app then costs one timeout rather than freezing the display. */
#define OMABAR_AX_TIMEOUT 0.15f

struct app_menu_slot {
    char *title;
    AXUIElementRef bar_item; /* the AXMenuBarItem */
    AXUIElementRef menu;      /* the AXMenu it owns */
};

struct app_menu_item_slot {
    char *title;
    char *shortcut;
    bool enabled;
    AXUIElementRef element; /* the AXMenuItem to press */
};

static struct app_menu_slot g_menus[OMABAR_MAX_APP_MENUS];
static int g_menu_count;
static pid_t g_pid;

static struct app_menu_item_slot g_items[OMABAR_MAX_APP_MENU_ITEMS];
static int g_item_count;
static int g_items_menu = -1;
static pid_t g_items_pid;
static bool g_force_read;

static void app_menu_items_clear(void) {
    for (int i = 0; i < g_item_count; i++) {
        free(g_items[i].title);
        free(g_items[i].shortcut);
        g_items[i].title = NULL;
        g_items[i].shortcut = NULL;
        if (g_items[i].element) {
            CFRelease(g_items[i].element);
            g_items[i].element = NULL;
        }
    }
    g_item_count = 0;
    g_items_menu = -1;
    g_items_pid = 0;
}

static void app_menus_clear(void) {
    app_menu_items_clear();
    for (int i = 0; i < g_menu_count; i++) {
        free(g_menus[i].title);
        g_menus[i].title = NULL;
        if (g_menus[i].bar_item) {
            CFRelease(g_menus[i].bar_item);
            g_menus[i].bar_item = NULL;
        }
        if (g_menus[i].menu) {
            CFRelease(g_menus[i].menu);
            g_menus[i].menu = NULL;
        }
    }
    g_menu_count = 0;
    g_pid = 0;
}

/* Copy a string-valued attribute into a malloc'd UTF-8 C string. */
static char *ax_copy_string(AXUIElementRef el, CFStringRef attribute) {
    if (!el) return NULL;
    CFTypeRef value = NULL;
    if (AXUIElementCopyAttributeValue(el, attribute, &value) != kAXErrorSuccess
        || !value)
        return NULL;

    char *out = NULL;
    if (CFGetTypeID(value) == CFStringGetTypeID()) {
        CFStringRef s = (CFStringRef)value;
        CFIndex cap = CFStringGetMaximumSizeForEncoding(CFStringGetLength(s),
                                                         kCFStringEncodingUTF8)
                      + 1;
        out = malloc((size_t)cap);
        if (out
            && !CFStringGetCString(s, out, (CFIndex)cap, kCFStringEncodingUTF8)) {
            free(out);
            out = NULL;
        }
    }
    CFRelease(value);
    return out;
}

static bool ax_copy_bool(AXUIElementRef el, CFStringRef attribute, bool fallback) {
    if (!el) return fallback;
    CFTypeRef value = NULL;
    if (AXUIElementCopyAttributeValue(el, attribute, &value) != kAXErrorSuccess
        || !value)
        return fallback;

    bool out = fallback;
    if (CFGetTypeID(value) == CFBooleanGetTypeID()) {
        out = CFBooleanGetValue((CFBooleanRef)value) ? true : false;
    } else if (CFGetTypeID(value) == CFNumberGetTypeID()) {
        int32_t n = 0;
        CFNumberGetValue((CFNumberRef)value, kCFNumberSInt32Type, &n);
        out = n != 0;
    }
    CFRelease(value);
    return out;
}

static int64_t ax_copy_int(AXUIElementRef el, CFStringRef attribute,
                           int64_t fallback) {
    if (!el) return fallback;
    CFTypeRef value = NULL;
    if (AXUIElementCopyAttributeValue(el, attribute, &value) != kAXErrorSuccess
        || !value)
        return fallback;

    int64_t out = fallback;
    if (CFGetTypeID(value) == CFNumberGetTypeID()) {
        int32_t n = 0;
        if (CFNumberGetValue((CFNumberRef)value, kCFNumberSInt32Type, &n))
            out = n;
    } else if (CFGetTypeID(value) == CFStringGetTypeID()) {
        /* some apps publish the modifier set as a string of glyphs */
        out = (int64_t)(uintptr_t)ax_copy_string(el, attribute);
    }
    CFRelease(value);
    return out;
}

/* Borrowed array of an element's children, or NULL. Caller releases. */
static CFArrayRef ax_children(AXUIElementRef el) {
    if (!el) return NULL;
    CFTypeRef value = NULL;
    if (AXUIElementCopyAttributeValue(el, kAXChildrenAttribute, &value)
            != kAXErrorSuccess
        || !value)
        return NULL;
    if (CFGetTypeID(value) != CFArrayGetTypeID()) {
        CFRelease(value);
        return NULL;
    }
    return (CFArrayRef)value;
}

/* Build a menu item's shortcut the way AppKit displays it: control, option,
   shift, command, then the key. The command key is implied unless the app
   says otherwise, which is why it is the one modifier with a "no" flag. */
static char *ax_copy_shortcut(AXUIElementRef el) {
    if (!el) return NULL;

    /* Prefer the app's own glyph; it already has the right glyphs and
       ordering for anything unusual. */
    char *glyph = ax_copy_string(el, kAXMenuItemCmdGlyphAttribute);
    if (glyph && glyph[0]) return glyph;
    free(glyph);

    char *key = ax_copy_string(el, kAXMenuItemCmdCharAttribute);
    if (!key || !key[0]) {
        free(key);
        return NULL;
    }

    int64_t mods = ax_copy_int(el, kAXMenuItemCmdModifiersAttribute, 0);

    char buf[64];
    size_t used = 0;
    buf[0] = '\0';
    if (mods & kAXMenuItemModifierControl)
        used += (size_t)snprintf(buf + used, sizeof(buf) - used, "⌃");
    if (mods & kAXMenuItemModifierOption)
        used += (size_t)snprintf(buf + used, sizeof(buf) - used, "⌥");
    if (mods & kAXMenuItemModifierShift)
        used += (size_t)snprintf(buf + used, sizeof(buf) - used, "⇧");
    if (!(mods & kAXMenuItemModifierNoCommand))
        used += (size_t)snprintf(buf + used, sizeof(buf) - used, "⌘");
    snprintf(buf + used, sizeof(buf) - used, "%s", key);
    free(key);
    return strdup(buf);
}

void app_menus_invalidate(void) { g_force_read = true; }

/* The SF Symbol has to be loaded on the main thread: NSImage is not safe to
   use from the bar's render thread, and asked there it hands back an image
   with no pixels rather than failing, which would silently draw nothing. */
static CGImageRef app_menus_apple_logo_load(void) {
    NSImage *symbol = [NSImage imageWithSystemSymbolName:@"apple.logo"
                                 accessibilityDescription:nil];
    if (!symbol) return NULL;

    CGRect rect = NSMakeRect(0, 0, symbol.size.width, symbol.size.height);
    CGImageRef img = [symbol CGImageForProposedRect:&rect context:nil hints:nil];
    if (!img || CGImageGetWidth(img) == 0 || CGImageGetHeight(img) == 0)
        return NULL;
    return img;
}

CGImageRef app_menus_apple_logo(void) {
    /* Cached rather than rebuilt per frame: the symbol never changes and the
       bar redraws far more often than the app list changes. Borrowed, so
       callers must not release it. */
    static CGImageRef cached = NULL;
    static bool tried = false;
    if (tried) return cached;

    if ([NSThread isMainThread]) {
        cached = app_menus_apple_logo_load();
    } else {
        dispatch_sync(dispatch_get_main_queue(), ^{
            cached = app_menus_apple_logo_load();
        });
    }

    /* Only latch once the symbol actually resolved, so a failed attempt (the
       main thread still starting up, say) can be retried later. */
    if (cached) tried = true;
    return cached;
}

CGImageRef app_menus_apple_logo_tinted(float r, float g, float b, float a) {
    /* Tint once and keep the result. The bar redraws every frame, so doing
       this per frame would be pure waste; the colour only changes if the theme
       is reloaded, so cache the last one and rebuild when it differs.

       The tint is baked into an offscreen bitmap rather than done on the bar's
       own context: filling through a template image needs a transparency
       layer, and layers on a flipped CGBitmapContext are not safe here. A
       fresh bitmap starts fully transparent, so a plain source-in fill clips
       to the symbol's alpha with no layer at all. Borrowed, like the template
       above, so callers must not release it. */
    static CGImageRef cached = NULL;
    static float last[4] = {-1.0f, -1.0f, -1.0f, -1.0f};

    if (cached && r == last[0] && g == last[1] && b == last[2] && a == last[3])
        return cached;

    CGImageRef tpl = app_menus_apple_logo();
    
    if (!tpl) return NULL;

    size_t w = CGImageGetWidth(tpl);
    size_t h = CGImageGetHeight(tpl);
    
    if (w == 0 || h == 0) return NULL;

    CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    if (!cs) return NULL;

    CGImageRef out = NULL;
    CGBitmapInfo info = (CGBitmapInfo)kCGImageAlphaPremultipliedLast;
    CGContextRef off = CGBitmapContextCreate(NULL, w, h, 8, 0, cs, info);
    
    CGColorRef tint = CGColorCreateGenericRGB(r, g, b, a);
    if (off && tint) {
        CGRect box = CGRectMake(0, 0, (CGFloat)w, (CGFloat)h);
        CGContextDrawImage(off, box, tpl);
        CGContextSetBlendMode(off, kCGBlendModeSourceIn);
        CGContextSetFillColorWithColor(off, tint);
        CGContextFillRect(off, box);
        out = CGBitmapContextCreateImage(off);
    }
    if (tint) CGColorRelease(tint);
    if (off) CGContextRelease(off);
    CGColorSpaceRelease(cs);
    if (!out) return NULL;

    if (cached) CGImageRelease(cached);
    cached = out;
    last[0] = r; last[1] = g; last[2] = b; last[3] = a;
    return cached;
}

int app_menus_refresh(void) {
    @autoreleasepool {
        pid_t pid = 0;
        NSRunningApplication *front = [[NSWorkspace sharedWorkspace] frontmostApplication];
        if (front) pid = [front processIdentifier];

        if (pid <= 0) {
            app_menus_clear();
            return -1;
        }
        if (pid == g_pid && g_menu_count > 0 && !g_force_read)
            return g_menu_count;

        app_menus_clear();
        g_pid = pid;
        g_force_read = false;

        AXUIElementRef app = AXUIElementCreateApplication(pid);
        if (!app) return -1;
        AXUIElementSetMessagingTimeout(app, OMABAR_AX_TIMEOUT);

        CFTypeRef bar = NULL;
        AXError err = AXUIElementCopyAttributeValue(app, kAXMenuBarAttribute, &bar);
        CFRelease(app);
        if (err != kAXErrorSuccess || !bar) {
            g_pid = 0;
            return -1;
        }

        CFArrayRef tops = ax_children((AXUIElementRef)bar);
        CFRelease(bar);
        if (!tops) {
            g_pid = 0;
            return -1;
        }

        CFIndex n = CFArrayGetCount(tops);
        if (n > OMABAR_MAX_APP_MENUS) n = OMABAR_MAX_APP_MENUS;
        for (CFIndex i = 0; i < n; i++) {
            AXUIElementRef el = (AXUIElementRef)CFArrayGetValueAtIndex(tops, i);
            if (!el) continue;

            AXUIElementRef menu = NULL;
            CFArrayRef sub = ax_children(el);
            if (sub) {
                if (CFArrayGetCount(sub) > 0) {
                    menu = (AXUIElementRef)CFArrayGetValueAtIndex(sub, 0);
                    if (menu) CFRetain(menu);
                }
                CFRelease(sub);
            }

            g_menus[g_menu_count].title = ax_copy_string(el, kAXTitleAttribute);
            g_menus[g_menu_count].bar_item = (AXUIElementRef)CFRetain(el);
            g_menus[g_menu_count].menu = menu;
            g_menu_count++;
        }
        CFRelease(tops);

        if (g_menu_count == 0) {
            g_pid = 0;
            return -1;
        }
        return g_menu_count;
    }
}

int app_menus_count(void) { return g_menu_count; }

const char *app_menus_title(int menu) {
    if (menu < 0 || menu >= g_menu_count) return "";
    return g_menus[menu].title ? g_menus[menu].title : "";
}

int app_menus_items_menu(void) { return g_items_menu; }

int app_menus_load_items(int menu) {
    @autoreleasepool {
        if (menu < 0 || menu >= g_menu_count || !g_menus[menu].menu) return -1;

        app_menu_items_clear();
        g_items_menu = menu;
        g_items_pid = g_pid;

        CFArrayRef leaves = ax_children(g_menus[menu].menu);
        if (!leaves) {
            g_items_menu = -1;
            return -1;
        }

        CFIndex n = CFArrayGetCount(leaves);
        if (n > OMABAR_MAX_APP_MENU_ITEMS) n = OMABAR_MAX_APP_MENU_ITEMS;
        for (CFIndex i = 0; i < n; i++) {
            AXUIElementRef el = (AXUIElementRef)CFArrayGetValueAtIndex(leaves, i);
            if (!el) continue;
            g_items[g_item_count].element = (AXUIElementRef)CFRetain(el);
            g_items[g_item_count].title = ax_copy_string(el, kAXTitleAttribute);
            g_items[g_item_count].shortcut = ax_copy_shortcut(el);
            g_items[g_item_count].enabled =
                ax_copy_bool(el, kAXEnabledAttribute, true);
            g_item_count++;
        }
        CFRelease(leaves);
        return g_item_count;
    }
}

int app_menus_item_count(void) { return g_item_count; }

const char *app_menus_item_title(int item) {
    if (item < 0 || item >= g_item_count) return "";
    return g_items[item].title ? g_items[item].title : "";
}

const char *app_menus_item_shortcut(int item) {
    if (item < 0 || item >= g_item_count) return "";
    return g_items[item].shortcut ? g_items[item].shortcut : "";
}

bool app_menus_item_enabled(int item) {
    if (item < 0 || item >= g_item_count) return false;
    return g_items[item].enabled;
}

/* A separator carries an AXMenuItem role and no title; keeping it in the list
   preserves the app's grouping, and the renderer turns it into a gap. */
bool app_menus_item_is_separator(int item) {
    if (item < 0 || item >= g_item_count) return false;
    const char *t = g_items[item].title;
    return (t == NULL || t[0] == '\0') && !g_items[item].enabled;
}

int app_menus_press(int item) {
    @autoreleasepool {
        if (item < 0 || item >= g_item_count) return -1;
        AXUIElementRef el = g_items[item].element;
        if (!el) return -1;
        if (g_items_pid != g_pid) return -1; /* the app moved on */
        return AXUIElementPerformAction(el, kAXPressAction) == kAXErrorSuccess
                   ? 0
                   : -1;
    }
}
