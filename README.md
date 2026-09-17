# omabar

A macOS menu bar (status bar) daemon written in C/Objective-C and configured
**entirely via Nix** — there is no CLI config language. You declare your bar in
a nix-darwin module and a binary config file is generated for the daemon.

Inspired by [SketchyBar](https://github.com/FelixKratz/SketchyBar), without the
Shell config. Everything — bar styling, items, events, animations, plugins — is
expressed as Nix options with type checking.

## Features

- Blurred, shadowed, top/bottom/left/right bar drawn with a borderless window.
- Nix module with a rich, typed option surface (`services.omabar`).
- Built-in components: clock, battery, volume, wifi, media, front app, spaces —
  enabled with one line each.
- Items are declarative: `icon`, `label`, `icon_strip`, `background`, padding,
  per-item fonts/colors, `update_mask` (event subscriptions), click scripts.
- Custom components built at runtime:
  - **alias** — captures the front app's window icon (hardware capture via
    `SLSHWCaptureSpace`) and follows it as the front app changes.
  - **graph** — live-updating area chart with configurable range, line/fill
    colors, and a data-source hook for script- or event-driven values.
- Swift **plugin SDK** for items backed by plugin executables the bar manages.
- Hot-reload support (`services.omabar.daemon.hotload`) via launchd `WatchPaths`.

## Quick start

omabar ships as a flake exposing a nix-darwin module:
`darwinModules.omabar` (alias of `nixosModules.omabar`).

In your flake:

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    nix-darwin.url = "github:LnL7/nix-darwin";
    nix-darwin.inputs.nixpkgs.follows = "nixpkgs";

    omabar = {
      url = "github:anomalyco/omabar";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.nix-darwin.follows = "nix-darwin";
    };
  };

  outputs = { nix-darwin, omabar, ... }: {
    darwinConfigurations.my-mac = nix-darwin.lib.darwinSystem {
      modules = [
        omabar.darwinModules.omabar
        {
          services.omabar.enable = true; # picks up the default theme
        }
      ];
    };
  };
}
```

`darwin-rebuild switch` builds the daemon (`pkgs.omabar`), installs the
launchd agent (RunAtLoad), and writes `/etc/omabar_config`.

## Configuration reference

Everything lives under `services.omabar`. All options have a default derived
from the default theme (`nix/default-theme.nix`) — turn the module on and you
get a functional dark bar with spaces, front-app, clock, volume, battery, wifi
and media items.

### Top-level options

| Option | Type | Default | Description |
| --- | --- | --- | --- |
| `enable` | bool | false | Turn on the daemon + launchd agent. |
| `theme` | path | `./default-theme.nix` | Importable theme expression; used as the seed defaults below. |
| `bar.position` | enum | `"top"` | `top` / `bottom` / `left` / `right`. |
| `bar.height` | int | 38 | Bar height in points. |
| `bar.width` | int | 0 | Width in points; `0` = full screen. |
| `bar.margin` | int | 0 | Margin from screen edges. |
| `bar.blur_radius` | int | 30 | Background blur (0 = off). |
| `bar.color` | hex string | `0x40000000` | Bar background, AARRGGBB. |
| `bar.shadow` / `shadow_color` | bool / hex | true / `0x40000000` | Window shadow. |
| `bar.topmost` / `sticky` | bool | true / true | Above other windows / all Spaces. |
| `bar.notch_width` / `notch_offset` | int | 0 / 0 | Notch handling on built-in displays. |
| `bar.y_offset` | int | 0 | Vertical offset. |
| `bar.alpha` | float | 1.0 | Window opacity. |
| `defaults.icon.*` / `defaults.label.*` | font + color | SF Pro Regular 13 / `0xfff2f4f8` | Default item font, color, highlight color. |
| `defaults.background.*` | color/border/radius | transparent / `0x33ffffff` / 6 | Item background defaults. |
| `defaults.padding_left/right` | int | 6 | Item padding defaults. |
| `defaultItem` | barItem | `{ }` | Template applied to every item before per-item options. |
| `items.left/right/center/center_left/center_right` | barItem attrs | theme sections | The bar sections. |
| `components.clock/battery/volume/wifi/media/front_app` | submodule | disabled | One-line component shortcuts (see below). |
| `animations.enable/duration/function` | bool/float/enum | false / 0.25 / `ease_out` | Item animation behavior. |
| `daemon.hotload` / `log_level` / `pid_file` / `lock_file` | … | false / `info` / `/tmp/omabar.pid` / `/tmp/omabar.lock` | Daemon behavior. |
| `fonts.packages` | list of pkg | `[ ]` | Fonts installed to `/Library/Fonts/Nix Fonts`. |
| `fonts.default` | str | `"SF Pro:Regular:13.0"` | Fallback font string. |
| `events.<name>«notification»` | submodule | `{ }` | Custom event registrations the daemon can subscribe to. |
| `plugins.<name>«package,update_interval,env,enable»` | submodule | `{ }` | Plugin executables managed by the bar. |
| `configFile` | path (read-only) | generated | Binary OMABC config produced for the daemon. |

### Bar items

An item is any attrset under `items.<section>`; each is typed by `type`:

> **Theme overrides**: the default theme's per-item fields act as a baseline.
> When you define an item, only the fields you set override the theme, and
> theme items you don't touch stay in the bar. Set a whole section to `{ }`
> (e.g. `items.center = { };`) to drop the theme's items there.

- `"clock"`, `"battery"`, `"volume"`, `"wifi"`, `"media"`, `"front_app"`,
  `"space"`, `"time"`, `"text"` (custom) — text/icon delegates.
- `type = ""` with `script` — runs a shell script to render the item.
- Alias and graph are *components* attached to any item (below).

Item options:

| Option | Type | Description |
| --- | --- | --- |
| `position` | `l`/`r`/`c`/`q`/`cl`/`cr` | Where the item sits in its section. |
| `type` | str | Component type (drives built-in rendering + event defaults). |
| `icon.font` / `icon.color` / `icon.string` | … | Icon glyph + styling. |
| `icon_strip` | list of str | Icon glyphs chosen by state (battery/volume/spaces). |
| `icon_highlight_color` | str | Color for the active/highlighted strip glyph. |
| `label.font` / `label.color` / `label.string` | … | Label text + styling. |
| `background` / `selected_background` | color, border, corner_radius | Item backgrounds. |
| `padding_left` / `padding_right` | int | Internal padding. |
| `y_offset` / `icon_x_offset` / `label_x_offset` | int | Fine positioning. |
| `update_interval` | int | Re-render every N ticks. |
| `update_mask` | list of event names | Events that re-render the item. |
| `associated_space` / `associated_display` | int | Scope the item to a Space/display (−1 = all). |
| `script` | str | Shell command rendering custom text. |
| `click_script` | str | Shell command run on click. |
| `scroll_enabled` / `click_enabled` | bool | Input enablement. |
| `mach_helper` | str | MACH helper bootstrap name (advanced). |

### Alias component

Captures the front app's menu-bar/window icon as a hardware snapshot
(`SLSHWCaptureSpace`, cropped to the window bounds) and keeps it in sync with
the front app:

```nix
services.omabar.items.center_left.front_app = {
  type = "front_app";
  alias = {
    bundle_id = "";        # restrict to a bundle ("" = follow front app)
    target_pid = "";       # or pin a specific process id
    owner = "";            # or window owner name
    name = "";             # or window name
    width = 0;             # 0 = keep captured size
    height = 0;
    corner_radius = 0;
    update_freq = 0;       # re-capture every N updates (0 = events only)
    inverse = false;
  };
};
```

Alias items auto-subscribe to `front_app_switched` and `scroll.tick`; the
capture refreshes when the front app changes.

### Graph component

An area/line chart item fed by a data source:

```nix
services.omabar.items.right.cpu = {
  type = "";               # plain item hosting the graph
  graph = {
    width = 48;
    height = 30;
    line_width = 1.0;
    fill_color = "0x2200aaff";   # optional area fill
    line_color = "0xffffffff";
    max_points = 64;
    min = 0.0;
    max = 1.0;
  };
  update_mask = [ "scroll.tick" ];   # graph auto-subscribes when set
};
```

Values are pushed from plugins or scripts; the C API exposes
`bar_item_graph_push`, `bar_item_graph_set_range` and
`bar_item_graph_set_data_source` for programmatic feeds.

### Components (shortcuts)

`components.clock.enable = true;` expands to a `clock` item on the right with
sane defaults. Component options: `icon`, `icon_strip`, `label_font`,
`label_color`, `format` (clock), `truncation` (media), `update_interval`.

> The default theme already ships clock / battery / volume / wifi / media /
> front-app as plain items, so prefer overriding `items` and only use
> `components` when starting from `items.right = { };` (etc.) — otherwise the
> component items and the themed items render twice.

## Plugins

`nix/plugin-sdk.nix` builds a header/yaml toolchain for Swift plugin
executables. Register a plugin with:

```nix
services.omabar.plugins.myplugin = {
  enable = true;
  package = pkgs.callPackage ./myplugin { };   # built with omabar-plugin-sdk
  update_interval = 5;
  env.OMABAR_DATA_DIR = "/tmp";
};
```

## Architecture

```
src/
  main.c            entry point: daemonize, lock file, runtime bootstrap
  bar_manager.c     owns the bar(s) and item lifecycle
  bar.c             window hosting, frame drawing, notch handling
  bar_item.c        item config + rendering + event subscription
  graph.c/h         live-data graph component (task 90)
  alias.c/h         front-app window capture component (task 89)
  app_windows.m     helper: bundle-id/pid/window resolution for alias
  event_loop.c      event dispatch (front app, spaces, media, volume, …)
  ...               font/color/animation/slider/mouse/popup helpers
nix/
  module.nix        nix-darwin module (services.omabar options)
  default-theme.nix default bar/items/fonts/animations theme
  config-generator.nix  module config -> OMABC binary config derivation
  package.nix       the C daemon (make -j4, Darwin SDK)
  plugin-sdk.nix    Swift plugin builder
```

The daemon compiles with `clang` + `apple-sdk_15` (see the flake devShell).
Local development: `cd src && make` → `src/bin/omabar`.

> **Status note**: the Nix side (module + OMABC config generator) is feature
> complete; the C config loader that consumes the generated blob is not yet
> wired in, so the daemon currently boots with empty defaults. Building on it
> is tracked in the project's SQLite task database (`omabar.db`).

## Development

- Tasks are tracked in `omabar.db` (SQLite). Use the repo `Makefile`:
  `make next`, `make start TASK=N`, `make done TASK=N`, `make note TASK=N MSG="…"`.
- Database schema + the original 94-step plan live in `create_db.sh`.

## License

MIT.