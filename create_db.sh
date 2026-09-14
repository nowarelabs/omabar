#!/bin/sh
# Create the omabar implementation tracking database
set -e

DB="omabar.db"

rm -f "$DB"

sqlite3 "$DB" <<'SQL'
-- Schema
CREATE TABLE tasks (
    id INTEGER PRIMARY KEY,
    phase INTEGER NOT NULL,
    phase_name TEXT NOT NULL,
    title TEXT NOT NULL,
    description TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'pending',  -- pending, in_progress, completed, blocked, cancelled
    priority TEXT NOT NULL DEFAULT 'medium', -- high, medium, low
    depends_on TEXT DEFAULT '',              -- comma-separated task IDs
    files_created TEXT DEFAULT '',           -- files this task creates
    files_modified TEXT DEFAULT '',          -- files this task modifies
    notes TEXT DEFAULT '',
    blocker TEXT DEFAULT '',
    created_at TEXT DEFAULT (datetime('now')),
    updated_at TEXT DEFAULT (datetime('now'))
);

CREATE TABLE task_logs (
    id INTEGER PRIMARY KEY,
    task_id INTEGER NOT NULL,
    action TEXT NOT NULL,  -- started, completed, blocked, note, subtask_created
    message TEXT NOT NULL,
    created_at TEXT DEFAULT (datetime('now')),
    FOREIGN KEY (task_id) REFERENCES tasks(id)
);

CREATE TABLE subtasks (
    id INTEGER PRIMARY KEY,
    parent_id INTEGER NOT NULL,
    title TEXT NOT NULL,
    description TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'pending',
    created_at TEXT DEFAULT (datetime('now')),
    FOREIGN KEY (parent_id) REFERENCES tasks(id)
);

-- Indexes
CREATE INDEX idx_tasks_status ON tasks(status);
CREATE INDEX idx_tasks_phase ON tasks(phase);
CREATE INDEX idx_task_logs_task_id ON task_logs(task_id);

-- ============================================================
-- PHASE 1: Foundation (Steps 1-15)
-- ============================================================

INSERT INTO tasks (id, phase, phase_name, title, description, priority, depends_on, files_created) VALUES
(1, 1, 'Foundation', 'Nix flake scaffold',
 'Create flake.nix with inputs (nixpkgs, flake-utils, nix-darwin) and outputs (packages.aarch64-darwin.omabar, packages.x86_64-darwin.omabar, nix-darwin modules.omabar). Set up the basic stdenv.mkDerivation that compiles C source with Apple frameworks. Verify `nix build` produces an empty binary.',
 'high', '', 'flake.nix, flake.lock'),

(2, 1, 'Foundation', 'Project directory structure',
 'Create the full src/ directory tree: src/, src/misc/, plugins/sdk/, default-theme/. Add placeholder files for each module (.c and .h stubs). Ensure the makefile can compile all stubs into a single binary that prints "omabar" and exits.',
 'high', '1', 'src/main.c, src/misc/defines.h, src/misc/helpers.h, src/misc/extern.h, src/Makefile'),

(3, 1, 'Foundation', 'Private framework declarations',
 'Port and adapt SketchyBar src/misc/extern.h to declare all private SkyLight, DisplayServices, and MediaRemote functions needed. Add compile-time guards for macOS version compatibility. Verify compilation succeeds with -framework SkyLight linkage.',
 'high', '2', 'src/misc/extern.h'),

(4, 1, 'Foundation', 'Helper utilities header',
 'Create src/misc/helpers.h with: string tokenization (port from SketchyBar helpers.h), rgba_color struct and hex parsing (from spacebar helpers.h), memory pool allocator (from spacebar memory_pool.h), stretchy buffer macros (from spacebar sbuffer.h), hash table (from spacebar hashtable.h), fork_exec helper, path resolution. All header-only with static inline functions.',
 'high', '3', 'src/misc/helpers.h'),

(5, 1, 'Foundation', 'Environment variable store',
 'Port SketchyBar env_vars.h: a key-value string store that plugins/scripts can read from. Implement env_vars_init, env_vars_set, env_vars_get, env_vars_destroy. Header-only with a global env_vars instance.',
 'medium', '4', 'src/misc/env_vars.h'),

(6, 1, 'Foundation', 'Compile-time constants',
 'Port and expand SketchyBar misc/defines.h: all string constants for property names, event names, component types, position types. Add Omabar-specific constants for Nix-generated config keys. Organize into logical sections with comments.',
 'medium', '3', 'src/misc/defines.h'),

(7, 1, 'Foundation', 'Main entry point',
 'Implement src/main.c: parse argv[0] for binary name, reject root, acquire lockfile (/tmp/omabar_$USER.lock via fcntl), obtain SLSMainConnectionID(), set up signal handlers, disable CG event suppression. Fork into daemon mode. Call bar_manager_init() and enter CFRunLoopRun(). No CLI argument parsing (Nix-only).',
 'high', '2, 4', 'src/main.c'),

(8, 1, 'Foundation', 'Makefile for local development',
 'Create src/Makefile: compile all .c and .m files, link with required frameworks (Carbon, AppKit, QuartzCore, CoreAudio, CoreWLAN, CoreVideo, IOKit, SkyLight, DisplayServices, MediaRemote). Support `make`, `make debug`, `make clean`, `make leak`, `make asan`. Universal binary target.',
 'high', '2', 'src/Makefile'),

(9, 1, 'Foundation', 'Event loop (lock-free MPSC queue)',
 'Adapt spacebar event_loop.c: mmap-based memory pool for event allocation, lock-free MPSC queue using __sync_bool_compare_and_swap, dedicated pthread consumer. Define event struct with type enum and void* data pointer. Implement event_loop_init, event_loop_begin, event_loop_post, event_loop_destroy.',
 'high', '4, 5', 'src/event_loop.{c,h}'),

(10, 1, 'Foundation', 'Event types and dispatch',
 'Define all event types in event.h (MACH_MESSAGE, DISPLAY_ADDED, DISPLAY_REMOVED, DISPLAY_MOVED, SPACE_CHANGED, WINDOW_FOCUSED, APP_FRONT_SWITCHED, APP_LAUNCHED, APP_TERMINATED, VOLUME_CHANGED, POWER_CHANGED, WIFI_CHANGED, MEDIA_CHANGED, BRIGHTNESS_CHANGED, MOUSE_CLICKED, MOUSE_ENTERED, MOUSE_EXITED, MOUSE_SCROLLED, SCROLL_TICK, ANIMATION_TICK, HOTLOAD, CUSTOM_EVENT, etc.). Implement event_handler[] dispatch table. All handlers run on main thread via dispatch_sync.',
 'high', '9', 'src/event.{c,h}'),

(11, 1, 'Foundation', 'Workspace event observer (Objective-C)',
 'Port SketchyBar workspace.m: NSWorkspace notification observers for activeSpaceDidChangeNotification, didWakeNotification, activeDisplayDidChangeNotification, menuBarDidChangeVisibilityNotification. Post events to event loop on each notification.',
 'high', '9, 10', 'src/workspace.{m,h}'),

(12, 1, 'Foundation', 'Display management',
 'Port SketchyBar display.c + spacebar display.c/display_manager.c: CGDisplayRegisterReconfigurationCallback, display_space_id via SLSManagedDisplayGetCurrentSpace, display_space_list via SLSCopyManagedDisplaySpaces, display bounds via CGDisplayBounds, display reconfiguration handler. NSScreen safe area insets (display_nsscreen.m) for notch detection.',
 'high', '9, 10, 11', 'src/display.{c,h}, src/display_nsscreen.{m,h}'),

(13, 1, 'Foundation', 'SkyLight window management',
 'Port SketchyBar window.c: window_open (SLSNewWindowWithOpaqueShapeAndContext, SLSSetWindowResolution, SLSSetWindowOpacity, SLWindowContextCreate, SLSSpaceCreate for sticky windows), window_close, window_set_frame, window_set_origin, window_order, windows_freeze/windows_unfreeze (SLSTransaction). Support for top/bottom/left/right bar positions and per-display windows.',
 'high', '12', 'src/window.{c,h}'),

(14, 1, 'Foundation', 'SkyLight surface + CALayer bridge',
 'Port SketchyBar surface.c + layer.m: SLSAddSurface, SLSSetSurfaceBounds, SLSBindSurface, CAContext contextWithCGSConnection, CALayer root layer assignment, CGBitmapContext creation (2x retina, premultiplied alpha). Surface_flush: CGBitmapContextCreateImage -> CALayer.contents -> SLSFlushSurface.',
 'high', '13', 'src/surface.{c,h}, src/layer.{m,h}, src/context.{c,h}'),

(15, 1, 'Foundation', 'Bitmap context for software rendering',
 'Port SketchyBar context.c: CGBitmapContextCreate with 8-bit premultiplied alpha, device RGB color space, 2x scale for retina. Provide context_create, context_destroy helpers. Test by drawing a solid color rectangle into a window.',
 'high', '14', 'src/context.{c,h}');

-- ============================================================
-- PHASE 2: Rendering Engine (Steps 16-30)
-- ============================================================

INSERT INTO tasks (id, phase, phase_name, title, description, priority, depends_on, files_created) VALUES
(16, 2, 'Rendering Engine', 'Color system',
 'Port color.c from SketchyBar: RGBA color struct, hex string parsing (0xAARRGGBB), color blending, color with alpha. Add Nix-compatible color format support. Header with inline functions for hot path.',
 'medium', '4', 'src/color.{c,h}'),

(17, 2, 'Rendering Engine', 'Font management',
 'Port font.c from SketchyBar: CTFont creation from "Family:Style:Size" string, font registration, font caching. Support for multiple font instances. Add font_destroy for cleanup.',
 'medium', '4, 16', 'src/font.{c,h}'),

(18, 2, 'Rendering Engine', 'Text rendering',
 'Port text.c from SketchyBar: CTLine creation from string + font, text measurement (ascent, descent, width), text drawing via CTLineDraw into CGContext. Support highlight color, text background, shadow. Add text_set_string, text_set_font, text_set_color.',
 'high', '17', 'src/text.{c,h}'),

(19, 2, 'Rendering Engine', 'Background drawing',
 'Port background.c from SketchyBar: rounded rectangle drawing via CGContext, solid color fill, border (color + width), corner_radius clipping, background image support, shadow drawing behind background. Support transparent backgrounds.',
 'high', '16', 'src/background.{c,h}'),

(20, 2, 'Rendering Engine', 'Shadow rendering',
 'Port shadow.c from SketchyBar: drop shadow with angle, distance, color. CGContext shadow drawing via CGContextSetShadowWithColor. Support per-component shadows.',
 'medium', '16', 'src/shadow.{c,h}'),

(21, 2, 'Rendering Engine', 'Image rendering',
 'Port image.c from SketchyBar: CGImage creation from file path, image drawing into CGContext with border and corner_radius. Support for image scaling. Add image_set_path, image_destroy.',
 'medium', '16', 'src/image.{c,h}'),

(22, 2, 'Rendering Engine', 'Line graph component',
 'Port graph.c from SketchyBar: line graph with configurable width/height, fill color, line color, data points. Graph drawing via CGContext path operations. graph_push_value for adding data points.',
 'medium', '18, 16', 'src/graph.{c,h}'),

(23, 2, 'Rendering Engine', 'Slider component',
 'Port slider.c from SketchyBar: horizontal slider with knob, foreground/background colors, mouse drag handling. Slider drawing and hit testing.',
 'medium', '18, 16', 'src/slider.{c,h}'),

(24, 2, 'Rendering Engine', 'Alias component (menu bar capture)',
 'Port alias.c from SketchyBar: capture another application menu bar item via SLSHWCaptureSpace or SLSCaptureWindowsContentsToRectWithOptions. Render captured image into bar item window. Support for targeting specific apps.',
 'medium', '14', 'src/alias.{c,h}'),

(25, 2, 'Rendering Engine', 'Group/bracket component',
 'Port group.c from SketchyBar: group items share a single background. When an item belongs to a group, its background is drawn by the group container. Implement group_add_item, group_calculate_bounds.',
 'medium', '19', 'src/group.{c,h}'),

(26, 2, 'Rendering Engine', 'Popup component',
 'Port popup.c from SketchyBar: popup menu anchored to a host item, containing child items. Popup_open, popup_close, popup drawing. Popup can appear above or below the host item.',
 'medium', '19, 25', 'src/popup.{c,h}'),

(27, 2, 'Rendering Engine', 'Animation engine',
 'Port animation.c from SketchyBar: CVDisplayLink-driven animation loop, interpolation functions (linear, ease-in, ease-out, ease-in-out, spring), animation struct with target, initial/final value, duration, update function pointer. animation_run, animation_update, animation_cancel.',
 'high', '10', 'src/animation.{c,h}'),

(28, 2, 'Rendering Engine', 'Mouse event handling',
 'Port mouse.c from SketchyBar: Carbon Event Manager handler for mouse events (click, drag, enter, exit, scroll). Hit testing against bar items. mouse_handler callback registration. Support for modifier keys.',
 'high', '10, 13', 'src/mouse.{c,h}'),

(29, 2, 'Rendering Engine', 'Custom events system',
 'Port custom_events.c from SketchyBar: user-defined events with bitmask tracking, NSDistributedNotification mapping. Register custom events by name, assign unique bit positions. custom_events_register, custom_events_trigger.',
 'medium', '10', 'src/custom_events.{c,h}'),

(30, 2, 'Rendering Engine', 'FSEvents hot-reload',
 'Port hotload.c from SketchyBar: FSEventStream watching config directory, rate-limited reload (1 second debounce). On change, post HOTLOAD event that destroys and reinitializes bar_manager. hotload_begin, hotload_end.',
 'medium', '10', 'src/hotload.{c,h}');

-- ============================================================
-- PHASE 3: Bar & Items (Steps 31-48)
-- ============================================================

INSERT INTO tasks (id, phase, phase_name, title, description, priority, depends_on, files_created) VALUES
(31, 3, 'Bar & Items', 'Bar item data structure',
 'Define bar_item.h: the fundamental unit with type enum (BAR_ITEM, BAR_COMPONENT_SPACE, BAR_COMPONENT_ALIAS, BAR_COMPONENT_GROUP, BAR_COMPONENT_GRAPH, BAR_COMPONENT_SLIDER), name string, icon (text), label (text), background, graph, alias, slider, group pointer, popup, position (l/r/c/q/e/p), associated_space/display/bar bitmasks, update_mask (64-bit event bitmask), script/click_script, mach_helper, signal_args.env_vars, hidden flag, click_enabled flag, scroll_enabled flag, y_offset, padding_left/right, label_x_offset, icon_x_offset.',
 'high', '18, 19', 'src/bar_item.{c,h}'),

(32, 3, 'Bar & Items', 'Bar item initialization and destruction',
 'Implement bar_item_init, bar_item_destroy, bar_item_clone. Default values from bar_manager.default_item. Deep copy for clone. Proper cleanup of all owned resources (text, background, popup, etc.).',
 'high', '31', 'src/bar_item.c'),

(33, 3, 'Bar & Items', 'Bar item property setters',
 'Implement all bar_item_set_* functions: set_name, set_icon (string -> CTLine), set_label (string -> CTLine), set_icon_font, set_label_font, set_icon_color, set_label_color, set_background_color, set_background_border_color, set_background_corner_radius, set_background_border_width, set_shadow, set_position, set_update_mask, set_script, set_click_script, set_hidden, set_click_enabled, set_scroll_enabled, set_y_offset, set_padding_left/right, set_label_x_offset, set_icon_x_offset, etc. Each setter marks the item as needing refresh.',
 'high', '32', 'src/bar_item.c'),

(34, 3, 'Bar & Items', 'Bar item drawing',
 'Implement bar_item_draw: given a CGContext and bounds, draw the items background, icon text, label text, graph, alias, slider. Handle highlight state. Respect hidden flag. Draw popup if open. Proper z-ordering of components.',
 'high', '31, 19, 18', 'src/bar_item.c'),

(35, 3, 'Bar & Items', 'Bar item bounds calculation',
 'Implement bar_item_calculate_bounds: compute the total width of an item (icon width + padding + label width + background padding). Handle center-aligned items. Return the bounds rect for the item within the bar.',
 'high', '31, 18', 'src/bar_item.c'),

(36, 3, 'Bar & Items', 'Bar item event handling',
 'Implement bar_item_update: called when an event matches the items update_mask. Update icon/label strings from environment variables. If the item has a script, fork_exec it with env vars. If it has a mach_helper, send via Mach IPC. Handle click and scroll events.',
 'high', '31, 10, 5', 'src/bar_item.c'),

(37, 3, 'Bar & Items', 'Bar data structure',
 'Define bar.h: per-display bar with window pointer, display ID, space ID, dsid, adid, shown/hidden/mouse_over flags, background (struct background for bar-wide bg), x_offset tracking for item placement.',
 'high', '13, 19', 'src/bar.{c,h}'),

(38, 3, 'Bar & Items', 'Bar creation and destruction',
 'Implement bar_create (allocate window, compute frame from display bounds + bar position + height + notch offset), bar_destroy (close window, free resources). One bar per display. Handle display hotplug (create/destroy bars on display add/remove).',
 'high', '37, 13, 12', 'src/bar.c'),

(39, 3, 'Bar & Items', 'Bar bounds calculation',
 'Implement bar_calculate_bounds: iterate all visible items, compute their bounds, arrange left/right/center items. Handle item positioning (left items grow right, right items grow left, center items centered). Account for padding, spacing, notch offset.',
 'high', '37, 35', 'src/bar.c'),

(40, 3, 'Bar & Items', 'Bar drawing',
 'Implement bar_draw: clear the bar window context, draw bar background (color, blur, shadow), iterate items in order and draw each at its computed position. Handle popup drawing. Flush surface after all items drawn.',
 'high', '37, 34, 39', 'src/bar.c'),

(41, 3, 'Bar & Items', 'Bar manager data structure',
 'Define bar_manager.h: central singleton owning bars[], bar_items[], default_item, custom_events, animator. Configuration: position, margin, blur_radius, shadow, topmost, sticky, notch_width/offset, y_offset, height, width. State: frozen, sleeps, bar_needs_update, bar_needs_resize, needs_ordering.',
 'high', '37, 31, 10, 27', 'src/bar_manager.{c,h}'),

(42, 3, 'Bar & Items', 'Bar manager initialization',
 'Implement bar_manager_init: set all defaults (position=top, height=40, margin=0, etc.), create default_item template, init custom_events, init animator. bar_manager_begin: create bars for all current displays, register event handlers, start animation loop.',
 'high', '41, 38', 'src/bar_manager.c'),

(43, 3, 'Bar & Items', 'Bar manager item management',
 'Implement bar_manager_add_item, bar_manager_remove_item, bar_manager_move_item, bar_manager_reorder_item, bar_manager_clone_item, bar_manager_rename_item. Items stored in ordered array. Find by name via linear scan (acceptable for typical item counts).',
 'high', '42, 31', 'src/bar_manager.c'),

(44, 3, 'Bar & Items', 'Bar manager property setters',
 'Implement bar_manager_set_*: set_position, set_height, set_width, set_margin, set_blur_radius, set_shadow, set_topmost, set_sticky, set_notch_width, set_notch_offset, set_y_offset, set_background_color, set_alpha. Each setter marks bar_needs_update or bar_needs_resize as appropriate.',
 'high', '42', 'src/bar_manager.c'),

(45, 3, 'Bar & Items', 'Bar manager refresh cycle',
 'Implement bar_manager_refresh: the core update cycle. If frozen, set bar_needs_update and return. Otherwise: recalculate bounds for all bars, redraw all bars, flush all surfaces. Rate-limit to display refresh via CVDisplayLink or timer. bar_manager_set_needs_update, bar_manager_set_needs_resize helpers.',
 'high', '42, 39, 40', 'src/bar_manager.c'),

(46, 3, 'Bar & Items', 'Bar manager event handlers',
 'Implement bar_manager handlers for all system events: handle_display_added/removed/moved (create/destroy/reposition bars), handle_space_changed (update space items), handle_window_focused (update front_app), handle_volume_changed, handle_power_changed, handle_wifi_changed, handle_media_changed, handle_brightness_changed, handle_mouse_clicked/entered/exited/scrolled. Each handler updates affected items and calls bar_manager_refresh.',
 'high', '42, 10', 'src/bar_manager.c'),

(47, 3, 'Bar & Items', 'Space component',
 'Implement BAR_COMPONENT_SPACE type in bar_item: track space ID, selected state, display association. Draw space icon from icon_strip or default glyph. Highlight selected space with background color. Handle space creation/destruction.',
 'high', '31, 12', 'src/bar_item.c'),

(48, 3, 'Bar & Items', 'Bar item script execution',
 'Implement the script execution pipeline: when bar_item_update triggers a script, set environment variables ($NAME, $SENDER, $INFO, $SELECTED, $SID, $DID, $BUTTON, $MODIFIER, $SCROLL_DELTA, custom vars), fork_exec the script, parse stdout for commands to send back to daemon. Handle mach_helper alternative for Mach IPC communication.',
 'high', '36, 5', 'src/bar_item.c');

-- ============================================================
-- PHASE 4: System Events (Steps 49-58)
-- ============================================================

INSERT INTO tasks (id, phase, phase_name, title, description, priority, depends_on, files_created) VALUES
(49, 4, 'System Events', 'Volume change events',
 'Port volume.c from SketchyBar: AudioObjectAddPropertyListener on kAudioHardwareServiceDeviceProperty_VirtualMainVolume. Fire VOLUME_CHANGED event with current volume percentage. volume_begin, volume_end.',
 'medium', '10', 'src/volume.{c,h}'),

(50, 4, 'System Events', 'Power source events',
 'Port power.c from SketchyBar: IOPSNotificationCreateRunLoopSource for power source changes. Fire POWER_CHANGED event with battery percentage and charging state. power_begin, power_end.',
 'medium', '10', 'src/power.{c,h}'),

(51, 4, 'System Events', 'WiFi change events',
 'Port wifi.c from SketchyBar: CoreWLAN CWInterface SSID change notifications. Fire WIFI_CHANGED event with new SSID. wifi_begin, wifi_end.',
 'medium', '10', 'src/wifi.{c,h}'),

(52, 4, 'System Events', 'Media now-playing events',
 'Port media.c from SketchyBar: MediaRemote framework for MRMediaRemoteRegisterForNowPlayingNotifications. Fire MEDIA_CHANGED event with title, artist, album, artwork. media_begin, media_end.',
 'medium', '10', 'src/media.{c,h}'),

(53, 4, 'System Events', 'Display brightness events',
 'Port display brightness from SketchyBar display.c: DisplayServicesRegisterForBrightnessChangeNotifications. Fire BRIGHTNESS_CHANGED event with current brightness level. display_brightness_begin, display_brightness_end.',
 'medium', '10, 12', 'src/display.c'),

(54, 4, 'System Events', 'App lifecycle events',
 'Implement application tracking: Carbon Event Manager for kEventAppLaunched, kEventAppTerminated, kEventAppFrontSwitched. Maintain process/application registry (hash table). Per-app Accessibility observers for AXFocusedWindowChanged, AXTitleChanged. Post APP_LAUNCHED, APP_TERMINATED, APP_FRONT_SWITCHED, WINDOW_FOCUSED events.',
 'high', '10, 4', 'src/app_windows.{c,h}'),

(55, 4, 'System Events', 'Space change events',
 'Implement space change detection: SLSManagedDisplayGetCurrentSpace polling or SkyLight notification procs (system events 904, 905, 1401, 1508, 1322, 1327, 1328). Post SPACE_CHANGED event with old/new space ID and display ID.',
 'high', '10, 12', 'src/app_windows.c'),

(56, 4, 'System Events', 'Event subscription system',
 'Implement event_subscribe on bar_item: when an item subscribes to an event, set the corresponding bit in update_mask. When an event fires, iterate all items and update those with matching bits. Support for: routine (timer), forced, and all system event types.',
 'high', '10, 31, 36', 'src/bar_item.c'),

(57, 4, 'System Events', 'Timer/tick events',
 'Implement a CVDisplayLink or CFRunLoopTimer-based tick source for periodic updates. Items with update_interval > 0 receive SCROLL_TICK events at the specified interval. Used for clock, battery, etc.',
 'medium', '10, 27', 'src/animation.c, src/bar_manager.c'),

(58, 4, 'System Events', 'Do Not Disturb detection',
 'Port spacebar dnd.c: detect DND state via Big Sur plist hack (com.apple.notificationcenterui prefs). Expose as a boolean event. Integrate with bar item display.',
 'low', '10', 'src/app_windows.c');

-- ============================================================
-- PHASE 5: Nix Configuration (Steps 59-73)
-- ============================================================

INSERT INTO tasks (id, phase, phase_name, title, description, priority, depends_on, files_created) VALUES
(59, 5, 'Nix Configuration', 'Nix flake outputs',
 'Expand flake.nix: add nix-darwin as input, define nixosModules.omabar (the nix-darwin module), overlay, packages for both architectures, devShell with build tools. Ensure `nix flake check` passes.',
 'high', '1', 'flake.nix'),

(60, 5, 'Nix Configuration', 'Module: bar options',
 'Create nix/module.nix with mkOption definitions for bar-level settings: services.omabar.enable, services.omabar.bar.position (top/bottom/left/right), services.omabar.bar.height, services.omabar.bar.width, services.omabar.bar.margin, services.omabar.bar.blur_radius, services.omabar.bar.color, services.omabar.bar.shadow (bool), services.omabar.bar.shadow_color, services.omabar.bar.topmost, services.omabar.bar.sticky, services.omabar.bar.notch_width, services.omabar.bar.notch_offset, services.omabar.bar.y_offset, services.omabar.bar.alpha.',
 'high', '59', 'nix/module.nix'),

(61, 5, 'Nix Configuration', 'Module: default item options',
 'Add to module.nix: services.omabar.defaults.icon.font, services.omabar.defaults.icon.color, services.omabar.defaults.icon.highlight_color, services.omabar.defaults.label.font, services.omabar.defaults.label.color, services.omabar.defaults.label.highlight_color, services.omabar.defaults.background.color, services.omabar.defaults.background.border_color, services.omabar.defaults.background.corner_radius, services.omabar.defaults.background.border_width, services.omabar.defaults.padding_left, services.omabar.defaults.padding_right.',
 'high', '60', 'nix/module.nix'),

(62, 5, 'Nix Configuration', 'Module: item definition type',
 'Define a freeform item type in module.nix using types.submodule: name (string), position (enum l/r/c/q/e), icon (submodule: font, color, highlight_color, string), label (submodule: font, color, highlight_color, string, format, y_offset), background (submodule: color, border_color, corner_radius, border_width, image, shadow), update_interval (int), update_mask (list of event names), associated_space (int), associated_display (int), script (string), click_script (string), y_offset (int), padding_left/right (int), label_x_offset (int), icon_x_offset (int), scroll_enabled (bool), click_enabled (bool), mach_helper (string).',
 'high', '61', 'nix/module.nix'),

(63, 5, 'Nix Configuration', 'Module: left/right/center item lists',
 'Add to module.nix: services.omabar.items.left (attrsOf item), services.omabar.items.right (attrsOf item), services.omabar.items.center (attrsOf item), services.omabar.items.center_left (attrsOf item), services.omabar.items.center_right (attrsOf item). Items are attrset where key is the item name.',
 'high', '62', 'nix/module.nix'),

(64, 5, 'Nix Configuration', 'Module: space component options',
 'Add space-specific options: services.omabar.items.left.spaces.type = "space", services.omabar.items.left.spaces.icon_strip (list of strings), services.omabar.items.left.spaces.icon.font/color, services.omabar.items.left.spaces.icon_highlight_color, services.omabar.items.left.spaces.background (color, corner_radius), services.omabar.items.left.spaces.selected_background, services.omabar.items.left.spaces.padding_left/right.',
 'medium', '63', 'nix/module.nix'),

(65, 5, 'Nix Configuration', 'Module: built-in component shortcuts',
 'Add convenience options that expand to full item definitions: services.omabar.items.right.clock (enable, format, update_interval, icon, label font/color), services.omabar.items.right.battery (enable, update_interval, icon_strip, label), services.omabar.items.right.volume (enable, icon_strip, label), services.omabar.items.right.wifi (enable, icon, label), services.omabar.items.right.media (enable, label, truncation), services.omabar.items.center.front_app (enable, icon, label), services.omabar.items.center.media (enable, label).',
 'high', '63', 'nix/module.nix'),

(66, 5, 'Nix Configuration', 'Module: plugin options',
 'Add plugin configuration: services.omabar.plugins (attrsOf plugin), where each plugin has: enable (bool), package (package), update_interval (int), env (attrsOf string). Plugins are Swift apps that get built with the plugin SDK.',
 'high', '63', 'nix/module.nix'),

(67, 5, 'Nix Configuration', 'Module: animation options',
 'Add animation config: services.omabar.animations.enable (bool), services.omabar.animations.duration (float, seconds), services.omabar.animations.function (enum linear/ease_in/ease_out/ease_in_out/spring).',
 'medium', '60', 'nix/module.nix'),

(68, 5, 'Nix Configuration', 'Module: daemon options',
 'Add daemon behavior: services.omabar.daemon.hotload (bool), services.omabar.daemon.log_level (enum debug/info/warn/error), services.omabar.daemon.pid_file (path), services.omabar.daemon.lock_file (path).',
 'medium', '60', 'nix/module.nix'),

(69, 5, 'Nix Configuration', 'Config file generator',
 'Create nix/config-generator.nix: a Nix derivation that takes the evaluated module options and generates a binary config file (not a shell script). The config file is a flat binary format that the daemon reads at startup. Contains all bar settings, item definitions, event subscriptions, and plugin paths. Use builtins.toJSON or a custom format for the config blob.',
 'high', '60, 61, 62, 63, 64, 65, 66, 67, 68', 'nix/config-generator.nix'),

(70, 5, 'Nix Configuration', 'Daemon startup service',
 'Add launchd agent generation in module.nix: services.omabar generates a ~/Library/LaunchAgents/com.omabar.daemon.plist that runs the omabar binary at login. Use launchd.packages or manual plist generation. Ensure proper environment (PATH for Nix-managed fonts, etc.).',
 'high', '69', 'nix/module.nix'),

(71, 5, 'Nix Configuration', 'Module: font management',
 'Add font options and Nix font integration: services.omabar.fonts.packages (list of nix font packages), services.omabar.fonts.default (string). The module should install fonts via home-manager or nix-darwin font mechanisms so they are available at runtime.',
 'medium', '60', 'nix/module.nix'),

(72, 5, 'Nix Configuration', 'Module: event subscriptions',
 'Add event configuration: services.omabar.events (attrsOf event), where each event has: name (string), notification (optional NSDistributedNotification name). These expand to custom_events registrations in the daemon.',
 'medium', '62, 29', 'nix/module.nix'),

(73, 5, 'Nix Configuration', 'Module: hot-reload support',
 'When services.omabar.daemon.hotload = true, the module should set up a file watcher on the Nix-generated config. Since Nix configs are immutable, hotload watches for nix-darwin reconfiguration and triggers a daemon restart/reload. Implement as a launchd watch path or a small helper script.',
 'medium', '70, 30', 'nix/module.nix');

-- ============================================================
-- PHASE 6: IPC & Plugins (Steps 74-83)
-- ============================================================

INSERT INTO tasks (id, phase, phase_name, title, description, priority, depends_on, files_created) VALUES
(74, 6, 'IPC & Plugins', 'IPC server (Unix socket)',
 'Implement IPC server using Unix domain sockets at /tmp/omabar_$USER.socket. Server listens on a dedicated thread, accepts connections, reads length-prefixed JSON messages, posts DAEMON_MESSAGE events to the event loop. Support for multiple concurrent plugin connections. socket_daemon_begin_un, socket_daemon_end.',
 'high', '10, 13', 'src/ipc.{c,h}'),

(75, 6, 'IPC & Plugins', 'IPC message protocol',
 'Define the JSON message protocol: {"type": "update", "item": "name", "icon": "...", "label": "...", "background_color": "..."}, {"type": "subscribe", "events": ["volume_changed", ...]}, {"type": "query", "item": "name"}, {"type": "trigger", "event": "custom_name", "data": {...}}. Implement message parsing and validation in ipc.c.',
 'high', '74', 'src/ipc.c'),

(76, 6, 'IPC & Plugins', 'IPC client (for daemon internal use)',
 'Implement ipc_client.c: connect to the Unix socket, send messages, receive responses. Used by the daemon for internal communication and by the CLI tool (if ever needed for debugging).',
 'medium', '74', 'src/ipc_client.{c,h}'),

(77, 6, 'IPC & Plugins', 'Plugin manager',
 'Implement plugin.c: track connected plugins (name, socket fd, subscribed events). When an event fires, notify all subscribed plugins. Route incoming plugin messages to bar_manager for item updates. Plugin registration, deregistration, health monitoring.',
 'high', '74, 75, 42', 'src/plugin.{c,h}'),

(78, 6, 'IPC & Plugins', 'Swift plugin SDK: protocol definition',
 'Create plugins/sdk/OmabarPlugin.swift: Swift protocol that plugin authors implement. Methods: `func onEvent(_ event: OmabarEvent) async`, `func onQuery(_ query: OmabarQuery) async -> OmabarItemState`. OmabarEvent enum with all system event types. OmabarQuery/OmabarItemState structs.',
 'high', '75', 'plugins/sdk/OmabarPlugin.swift'),

(79, 6, 'IPC & Plugins', 'Swift plugin SDK: IPC client',
 'Create plugins/sdk/OmabarClient.swift: Swift class that handles Unix socket connection to the daemon. Methods: connect(), send(_ message: OmabarMessage), subscribe(to events: [OmabarEvent]), update(item: String, icon: String?, label: String?, backgroundColor: String?). Async/await based.',
 'high', '74, 78', 'plugins/sdk/OmabarClient.swift'),

(80, 6, 'IPC & Plugins', 'Swift plugin SDK: Nix derivation',
 'Create nix/plugin-sdk.nix: a Nix derivation that provides the Swift plugin SDK as a Swift Package. Includes OmabarPlugin.swift, OmabarClient.swift, and any helper types. Used by plugin authors as a build input.',
 'high', '78, 79, 59', 'nix/plugin-sdk.nix'),

(81, 6, 'IPC & Plugins', 'Swift plugin SDK: helper libraries',
 'Create Swift helper modules in the SDK for common macOS APIs: OmabarBrightness (DisplayServices wrapper), OmabarMedia (MediaRemote wrapper), OmabarNetwork (CoreWLAN wrapper), OmabarVolume (CoreAudio wrapper), OmabarBattery (IOKit wrapper). Each wraps the corresponding C API in a Swift-friendly interface.',
 'medium', '80', 'plugins/sdk/Helpers/'),

(82, 6, 'IPC & Plugins', 'Example plugin: battery',
 'Create plugins/battery/main.swift: a Swift plugin that monitors battery via IOKit, sends periodic updates to the daemon with icon and label. Demonstrates the full plugin lifecycle: connect, subscribe to timer, update item.',
 'medium', '78, 79', 'plugins/battery/'),

(83, 6, 'IPC & Plugins', 'Example plugin: media now-playing',
 'Create plugins/media/main.swift: a Swift plugin that monitors MediaRemote for now-playing info, sends updates with title/artist/album. Demonstrates event subscription and rich item updates.',
 'medium', '78, 79, 81', 'plugins/media/');

-- ============================================================
-- PHASE 7: Advanced Features (Steps 84-90)
-- ============================================================

INSERT INTO tasks (id, phase, phase_name, title, description, priority, depends_on, files_created) VALUES
(84, 7, 'Advanced Features', 'Notch-aware layout',
 'Implement notch offset calculation: use NSScreen.safeAreaInsets to detect MacBook notch, offset bar items to avoid the notch area. Support notch_width and notch_offset Nix options. Handle displays with and without notches.',
 'high', '12, 39', 'src/bar.c, src/display_nsscreen.m'),

(85, 7, 'Advanced Features', 'Multi-monitor support',
 'Ensure proper multi-monitor behavior: one bar per display, items can be associated with specific displays via associated_display bitmask, space items show correct spaces per display, display add/remove dynamically creates/destroys bars. Test with display reconfiguration callbacks.',
 'high', '38, 46, 12', 'src/bar_manager.c, src/bar.c'),

(86, 7, 'Advanced Features', 'Scroll animation (menu bar scrubbing)',
 'Implement scroll-based animation: when user scrolls on a bar item, trigger smooth animation through a list of values (e.g., cycling through media, volume levels). CVDisplayLink-driven smooth interpolation. Configurable via scroll_delta sensitivity.',
 'medium', '27, 28, 56', 'src/bar_item.c, src/animation.c'),

(87, 7, 'Advanced Features', 'Popup menu system',
 'Full popup implementation: items can have child items displayed in a popup menu. Popup appears on click, positioned relative to host item. Support popup Background, border, corner_radius. Click outside closes popup. Multiple popup nesting support.',
 'medium', '26, 31, 28', 'src/popup.c, src/bar_item.c'),

(88, 7, 'Advanced Features', 'Slider interaction',
 'Full slider support: horizontal slider with mouse drag. Slider updates value in real-time as user drags. Visual feedback with foreground fill and knob position. Custom events fired on value change. Nix-configurable min/max/initial value.',
 'medium', '23, 28', 'src/slider.c, src/bar_item.c'),

(89, 7, 'Advanced Features', 'Alias component (app menu bar capture)',
 'Full alias implementation: capture another applications menu bar item rendering via SLSHWCaptureSpace. Display the captured image as an icon in the bar. Update capture when target app changes. Support for targeting specific apps by bundle ID.',
 'medium', '24, 54', 'src/alias.c'),

(90, 7, 'Advanced Features', 'Graph component with live data',
 'Full graph implementation: line graph with configurable dimensions, colors, and data buffer. graph_push_value adds a data point, old points scroll off. Multiple graphs can display different data streams. CVDisplayLink-driven redraw for smooth animation.',
 'medium', '22, 27', 'src/graph.c');

-- ============================================================
-- PHASE 8: Default Theme & Polish (Steps 91-94)
-- ============================================================

INSERT INTO tasks (id, phase, phase_name, title, description, priority, depends_on, files_created) VALUES
(91, 8, 'Default Theme & Polish', 'Default theme definition',
 'Create nix/default-theme.nix: a complete, opinionated dark theme. Colors: translucent dark background (0x40000000), white text (0xffffffff), accent color for selections (0xff40a0ff), subtle borders. Fonts: SF Pro family (system font) with appropriate weights for icon/label. Layout: spaces on left, front_app center-left, clock/battery/volume/wifi on right. Item spacing, padding, corner_radius tuned for visual harmony.',
 'high', '65, 69', 'nix/default-theme.nix'),

(92, 8, 'Default Theme & Polish', 'Default space icons',
 'Define default space icon strip in the theme: elegant Unicode/glyph icons for spaces 1-10. Selected state uses accent color, unselected uses muted color. Smooth transition animation on space change.',
 'medium', '91, 47', 'nix/default-theme.nix'),

(93, 8, 'Default Theme & Polish', 'Integration testing',
 'End-to-end test: build omabar with `nix build`, verify the binary runs, verify it connects to SkyLight, verify bars appear on screen, verify items render correctly, verify IPC works with a test plugin, verify Nix module evaluates without errors. Document any macOS version-specific issues.',
 'high', '91, 92, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90', ''),

(94, 8, 'Default Theme & Polish', 'Documentation and examples',
 'Create README.md with: project overview, installation instructions (nix flake), configuration reference (all Nix options), plugin development guide (Swift SDK), architecture overview, contributing guide. Create example configurations for common setups (minimal, full, multi-monitor).',
 'medium', '93', 'README.md, examples/');

SQL

echo "Database created: $DB"
echo "Total tasks: $(sqlite3 "$DB" 'SELECT COUNT(*) FROM tasks')"
echo "Phases: $(sqlite3 "$DB" 'SELECT COUNT(DISTINCT phase) FROM tasks')"
