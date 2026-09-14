# Omabar — Project Vision & Implementation Prompt

## Vision

Omabar is a macOS status bar that replaces SketchyBar and Spacebar. It is built
in C (with Objective-C for AppKit/QuartzCore interop) using the same private
macOS frameworks (SkyLight, DisplayServices, MediaRemote) that power SketchyBar,
but with a fundamentally different configuration philosophy: **Nix-only**.

There are no CLI commands for end users. No shell-script config files. No
`sketchybar --set` invocations. Everything — from bar appearance to item
definitions to event subscriptions to plugin management — is expressed as Nix
flake options and compiled into the daemon at build time.

## Core Principles

1. **Nix-only configuration.** The daemon reads a compiled-in configuration
   generated from Nix module options. No runtime config files, no CLI toggle
   commands. `nix run` and you're done.

2. **All SketchyBar features.** Every visual capability of SketchyBar is
   present: items with icon/label/background, graphs, sliders, aliases, popups,
   groups, animations, shadows, borders, rounded corners, images, per-display
   bars, sticky windows, notch awareness, space tracking, and more.

3. **Swift plugin system.** Plugins are native Swift macOS desktop apps that
   communicate with the daemon via a well-defined IPC protocol (Unix socket or
   Mach ports). Nix provides a plugin SDK with helper libraries for common macOS
   APIs (brightness, media, network, etc.).

4. **Opinionated defaults.** Out of the box, Omabar is beautiful. A curated
   dark theme with carefully chosen fonts, colors, spacing, and item layout.
   Users customize by overriding Nix options, not by writing config from scratch.

5. **World-class architecture.** Clean C code following the patterns of the best
   C codebases: clear module boundaries, header-only utilities where appropriate,
   a central event loop, and zero unnecessary dependencies.

## Reference Codebases

The `SketchyBar/` and `spacebar/` directories contain reference implementations.
Omabar borrows heavily from both:

- **From SketchyBar**: the item model (bar_item with icon/label/background/graph/alias/slider/popup),
  the CALayer-based rendering pipeline, the event dispatch system, the animation
  engine, mouse handling, all subsystem event sources (volume, power, wifi, media,
  display), and the SkyLight window management approach.

- **From Spacebar**: the Nix flake integration pattern, the simpler event loop
  design (lock-free MPSC queue), the memory pool allocator, the unity build
  approach, and the display/space query helpers.

## Architecture Overview

```
omabar/
  flake.nix              # Nix flake: package + nix-darwin module
  nix/
    module.nix           # nix-darwin module with all options
    default-theme.nix    # Opinionated default theme
    plugin-sdk.nix       # Swift plugin SDK derivation
  src/
    main.c               # Entry point, daemon bootstrap
    event_loop.{c,h}     # Lock-free MPSC event queue + worker
    event.{c,h}          # Event types, handlers, dispatch
    bar_manager.{c,h}    # Central orchestrator
    bar.{c,h}            # Per-display bar instance
    bar_item.{c,h}       # Individual bar item
    window.{c,h}         # SkyLight window management
    surface.{c,h}        # SkyLight surface + CALayer bridge
    context.{c,h}        # CGBitmapContext creation
    layer.{m,h}          # Objective-C CALayer/CAContext bridge
    text.{c,h}           # CoreText text rendering
    font.{c,h}           # CTFont management
    color.{c,h}          # RGBA color handling
    background.{c,h}     # Background drawing
    shadow.{c,h}         # Drop shadow rendering
    image.{c,h}          # Image loading/drawing
    graph.{c,h}          # Line graph component
    alias.{c,h}          # Menu bar alias capture
    slider.{c,h}         # Interactive slider
    group.{c,h}          # Bracket/group component
    popup.{c,h}          # Popup menu
    animation.{c,h}      # CVDisplayLink animation engine
    mouse.{c,h}          # Carbon Event mouse handling
    workspace.{m,h}      # NSWorkspace observers
    display.{c,h}        # Display management
    display_nsscreen.{m,h} # NSScreen safe area insets
    volume.{c,h}         # CoreAudio volume events
    power.{c,h}          # IOKit power events
    wifi.{c,h}           # CoreWLAN wifi events
    media.{c,h}          # MediaRemote now-playing
    app_windows.{c,h}    # Space window tracking
    hotload.{c,h}        # Config hot-reloading (FSEvents)
    custom_events.{c,h}  # User-defined events
    ipc.{c,h}            # IPC server (Unix socket / Mach)
    plugin.{c,h}         # Plugin manager
    misc/
      defines.h          # Constants and string definitions
      helpers.h          # Utility functions
      extern.h           # Private framework declarations
      env_vars.h         # Environment variable store
  plugins/
    sdk/
      OmabarPlugin.swift # Swift plugin protocol
      OmabarClient.swift # Swift IPC client
    battery/             # Example: battery plugin
    media/               # Example: media now-playing plugin
  default-theme/
    theme.toml           # Default theme definition
```

## Nix Configuration Surface

The entire bar is configured through Nix options. Example:

```nix
{
  services.omabar = {
    enable = true;

    bar = {
      position = "top";
      height = 40;
      blur_radius = 30;
      color = "0x40000000";
      shadow = true;
      notch_width = 120;
    };

    defaults = {
      icon = {
        font = "SF Pro:Bold:14.0";
        color = "0xffffffff";
      };
      label = {
        font = "SF Pro:Regular:13.0";
        color = "0xffffffff";
      };
    };

    items = {
      left = {
        spaces = {
          icon = {
            font = "SF Pro:Bold:16.0";
          };
          background = {
            color = "0x40ffffff";
            corner_radius = 5;
          };
        };
      };
      center = {
        front_app = {
          label = {
            font = "SF Pro:Semibold:14.0";
            color = "0xffffffff";
          };
        };
      };
      right = {
        clock = {
          label = {
            format = "%a %d %b  %H:%M:%S";
          };
          update_interval = 1;
        };
        volume = {};
        battery = {};
        wifi = {};
      };
    };

    plugins = {
      battery = {
        enable = true;
        update_interval = 30;
      };
      media = {
        enable = true;
      };
    };

    animations = {
      enable = true;
      duration = 0.3;
    };
  };
}
```

## 94-Step Implementation Plan

The implementation is tracked in `omabar.db` (SQLite). Steps are organized into
8 phases. Use the Makefile to query progress, find next tasks, and manage work:

```bash
make next          # Show the next unblocked task
make status        # Show overall progress
make blocked       # Show blocked tasks
make log TASK=1    # View logs for a task
make start TASK=1  # Mark a task as in_progress
make done TASK=1   # Mark a task as completed
make fail TASK=1 BLOCKER="reason"  # Mark as blocked
make subtasks TASK=1  # Break down a task
```

## IPC Protocol for Plugins

Plugins communicate with Omabar via a simple JSON-based protocol over Unix
sockets or Mach ports. The protocol supports:

- `update` — push new values for an item's icon/label/background
- `subscribe` — subscribe to system events (volume, media, power, etc.)
- `query` — query current state of items
- `trigger` — trigger custom events

Nix provides a Swift SDK (`OmabarPlugin` protocol) that handles all IPC
plumbing. Plugin developers implement a single protocol conforming type and
Nix builds it into a `.app` bundle.

## What Makes Omabar Different

| Feature | SketchyBar | Spacebar | Omabar |
|---------|-----------|----------|--------|
| Configuration | Shell script CLI | Shell script CLI | **Nix options** |
| Plugin language | Shell scripts | None | **Swift apps** |
| Default theme | None (user builds) | None (minimal) | **Opinionated, beautiful** |
| Architecture | 68 files, flat | Unity build | **Clean modular C** |
| Build system | Makefile | Makefile | **Nix flake** |
| Mouse support | Full | None | **Full** |
| Animations | CVDisplayLink | None | **CVDisplayLink** |
| Items | Generic | Hardcoded | **Generic + Nix-typed** |
| Popups | Yes | No | **Yes** |
| Graphs | Yes | No | **Yes** |
| Sliders | Yes | No | **Yes** |
| Hot-reload | FSEvents | None | **FSEvents** |
