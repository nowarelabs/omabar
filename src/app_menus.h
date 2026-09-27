#ifndef OMABAR_APP_MENUS_H
#define OMABAR_APP_MENUS_H

#include <stdbool.h>
#include <CoreGraphics/CGImage.h>

/* The frontmost app's menu bar, read through the Accessibility API.

   omabar draws over the native menu bar, so the app menus macOS would show
   there have to be re-hosted in our own bar. Reading them is the same data
   assistive tools use: the app element's `AXMenuBar`, its `AXMenuBarItem`
   children, and each of those' `AXMenu`.

   Two properties make this practical rather than merely possible:

   - `AXPress` on a menu item succeeds while its menu is *closed*, so omabar
     can run a real menu command without ever surfacing the hidden native bar
     and without synthesising keystrokes.
   - The expensive half is the leaves, so they are read on click, for one
     menu at a time, rather than up front for every menu.

   Accessibility permission is already required for the click tap, so this
   costs the user nothing extra. */

#define OMABAR_MAX_APP_MENUS 16
#define OMABAR_MAX_APP_MENU_ITEMS 96

/* The Apple logo as a template image, drawn small next to the text. Borrowed
   and cached for the life of the process: NULL if the symbol is unavailable. */
CGImageRef app_menus_apple_logo(void);
CGImageRef app_menus_apple_logo_tinted(float r, float g, float b, float a);

/* Menu indices with a fixed meaning in every app's menu bar. Index 0 is the
   system Apple menu and index 1 is the app's own menu, so both can be
   addressed without matching on a localized title. */
#define OMABAR_APP_MENU_APPLE 0
#define OMABAR_APP_MENU_APP   1
#define OMABAR_APP_MENU_FIRST 2  /* first app-provided menu ("File", ...) */

/* Force the next refresh to re-read even if the front app has not changed
   (an app can add or rename a top-level menu at runtime). */
void app_menus_invalidate(void);

/* Re-read the frontmost app's menu bar and return the number of top-level
   menus, or -1 if they could not be read. Cheap to call repeatedly: it
   returns the cache while the front app is unchanged. */
int app_menus_refresh(void);

int app_menus_count(void);
const char *app_menus_title(int menu);

/* Leaves of one top-level menu, read on demand. Return -1 if unreadable, in
   which case the caller should draw the title with no menu attached. */
int app_menus_load_items(int menu);
int app_menus_items_menu(void);

int app_menus_item_count(void);
const char *app_menus_item_title(int item);
const char *app_menus_item_shortcut(int item);
bool app_menus_item_enabled(int item);
bool app_menus_item_is_separator(int item);

/* Perform a leaf. Returns 0 on success. */
int app_menus_press(int item);

#endif
