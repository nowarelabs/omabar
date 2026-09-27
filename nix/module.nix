{ config, lib, pkgs, ... }:

with lib;

let
  cfg = config.services.omabar;

  theme = import ./default-theme.nix;

  configGenerator = import ./config-generator.nix { inherit lib pkgs; };

  # A copy of the signed bundle at a stable, user-visible path, used as the
  # target of the Accessibility grant. It has to be a real copy rather than a
  # symlink into /nix/store: System Settings' file picker cannot navigate into
  # /nix/store, and the "+" button that adds an app to the Accessibility list
  # will not accept a store path. ~/Applications is the conventional place for
  # a per-user app that needs a permission grant, and because the copy is signed
  # with a build-independent identifier the grant survives rebuilds.
  # This is a nix-darwin module, so there is no Home Manager `home` option in
  # scope. nix-darwin's system.primaryUser is just the username string; the
  # home directory lives in that user's record.
  primaryUser = config.system.primaryUser;
  omabarAppDir =
    if primaryUser == null then "/Applications"
    else "${config.users.users.${primaryUser}.home}/Applications";
  omabarApp = "${omabarAppDir}/Omabar.app";
  omabarBin = "${omabarApp}/Contents/MacOS/Omabar";

  colorType = types.str; # "0xAARRGGBB" | "#RRGGBB[AA]" | bare hex

  fontType = types.submodule {
    options.font = mkOption {
      type = types.str;
      default = "";
      description = "Font string (family:style:size).";
    };
    options.color = mkOption {
      type = colorType;
      default = "";
      description = "Font color.";
    };
    options.highlight_color = mkOption {
      type = colorType;
      default = "";
      description = "Highlighted font color.";
    };
    options.string = mkOption {
      type = types.str;
      default = "";
      description = "Static string for this text.";
    };
  };

  textType = types.submodule {
    options.font = mkOption {
      type = types.str;
      default = "";
      description = "Font string (family:style:size).";
    };
    options.color = mkOption {
      type = colorType;
      default = "";
      description = "Text color.";
    };
    options.highlight_color = mkOption {
      type = colorType;
      default = "";
      description = "Highlighted text color.";
    };
    options.string = mkOption {
      type = types.str;
      default = "";
      description = "Static string for this text.";
    };
    options.format = mkOption {
      type = types.str;
      default = "";
      description = "Format string applied to the content.";
    };
    options.y_offset = mkOption {
      type = types.int;
      default = 0;
      description = "Vertical offset of the text in points.";
    };
    options.x_offset = mkOption {
      type = types.int;
      default = 0;
      description = "Horizontal offset of the text in points.";
    };
  };

  backgroundType = types.submodule {
    options.color = mkOption {
      type = colorType;
      default = "";
      description = "Background color.";
    };
    options.border_color = mkOption {
      type = colorType;
      default = "";
      description = "Background border color.";
    };
    options.corner_radius = mkOption {
      type = types.int;
      default = 0;
      description = "Corner radius in points.";
    };
    options.border_width = mkOption {
      type = types.int;
      default = 0;
      description = "Border width in points.";
    };
    options.image = mkOption {
      type = types.nullOr types.str;
      default = null;
      description = "Image path for the background.";
    };
    options.shadow = mkOption {
      type = types.nullOr (types.submodule {
        options.blur_radius = mkOption {
          type = types.float;
          default = 0.0;
          description = "Shadow blur radius.";
        };
        options.color = mkOption {
          type = colorType;
          default = "";
          description = "Shadow color.";
        };
        options.offset = mkOption {
          type = types.ints.positive;
          default = 0;
          description = "Shadow position offset.";
        };
      });
      default = null;
      description = "Background shadow settings.";
    };
  };

  barItemType = types.submodule ({ name, ... }: {
    options.name = mkOption {
      type = types.str;
      default = name;
      description = "Unique item name (defaults to the attr key in item lists).";
    };
    options.position = mkOption {
      type = types.nullOr (types.enum [ "l" "r" "c" "q" "e" ]);
      default = null;
      description = "Bar section: l=left, r=right, c=center, q=quarter, e=edge; null inherits the theme.";
    };
    options.icon = mkOption {
      type = types.nullOr fontType;
      default = null;
      description = "Item icon (defaults or overrides per item).";
    };
    options.label = mkOption {
      type = types.nullOr textType;
      default = null;
      description = "Item label (defaults or overrides per item).";
    };
    options.background = mkOption {
      type = types.nullOr backgroundType;
      default = null;
      description = "Item background (defaults or overrides per item).";
    };
    options.update_interval = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Re-run the script every N scroll ticks; 0 = on event only; null inherits the theme.";
    };
    options.update_mask = mkOption {
      type = types.nullOr (types.listOf types.str);
      default = null;
      description = "Event names that trigger this item's script; null inherits the theme.";
    };
    options.associated_space = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Only show on this Space (-1 = all; null inherits the theme).";
    };
    options.associated_display = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Only show on this display (-1 = all; null inherits the theme).";
    };
    options.script = mkOption {
      type = types.nullOr types.str;
      default = null;
      description = "ScriptPath or shell command to produce the item content.";
    };
    options.click_script = mkOption {
      type = types.nullOr types.str;
      default = null;
      description = "Script command run on click.";
    };
    options.y_offset = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Vertical offset of the item in points.";
    };
    options.padding_left = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Left padding in points.";
    };
    options.padding_right = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Right padding in points.";
    };
    options.label_x_offset = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Horizontal offset of the label in points.";
    };
    options.icon_x_offset = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Horizontal offset of the icon in points.";
    };
    options.scroll_enabled = mkOption {
      type = types.nullOr types.bool;
      default = null;
      description = "Whether the item reacts to scroll events.";
    };
    options.click_enabled = mkOption {
      type = types.nullOr types.bool;
      default = null;
      description = "Whether the item reacts to click events.";
    };
    options.mach_helper = mkOption {
      type = types.nullOr types.str;
      default = null;
      description = "Mach helper (background light) to install.";
    };
    options.type = mkOption {
      type = types.nullOr types.str;
      default = null;
      description = "Component type; empty = plain script item, space = space component; null inherits the theme.";
    };
    options.icon_strip = mkOption {
      type = types.nullOr (types.listOf types.str);
      default = null;
      description = "Icon strip for the space component (per-space icons).";
    };
    options.icon_highlight_color = mkOption {
      type = types.nullOr colorType;
      default = null;
      description = "Highlighted icon color used for selected spaces.";
    };
    options.space_gap = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Gap in points between space component chips; null inherits the theme.";
    };
    options.format = mkOption {
      type = types.nullOr types.str;
      default = null;
      description = "strftime format for a clock item; null inherits the theme.";
    };
    options.divider_width = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Divider thickness in points for a divider item; null inherits the theme.";
    };
    options.divider_color = mkOption {
      type = types.nullOr colorType;
      default = null;
      description = "Divider color (hex, AARRGGBB) for a divider item; null inherits the theme.";
    };
    options.order = mkOption {
      type = types.nullOr types.int;
      default = null;
      description = "Sort key for items inside a bar section; lower comes first.";
    };
    options.selected_background = mkOption {
      type = types.nullOr (types.submodule {
        options.color = mkOption {
          type = colorType;
          default = "0x00000000";
          description = "Selected space background color.";
        };
        options.corner_radius = mkOption {
          type = types.int;
          default = 0;
          description = "Selected space background corner radius in points.";
        };
        options.border_color = mkOption {
          type = colorType;
          default = "";
          description = "Selected space border color.";
        };
        options.border_width = mkOption {
          type = types.int;
          default = 0;
          description = "Selected space border width in points.";
        };
      });
      default = null;
      description = "Background used for the selected space.";
    };
    options.alias = mkOption {
      type = types.nullOr (types.submodule {
        options.target_pid = mkOption {
          type = types.str;
          default = "";
          description = "Capture the menu bar of this PID's app.";
        };
        options.bundle_id = mkOption {
          type = types.str;
          default = "";
          description = "Capture the menu bar of the app with this bundle id.";
        };
        options.owner = mkOption {
          type = types.str;
          default = "";
          description = "Capture the menu bar of the app with this process name.";
        };
        options.name = mkOption {
          type = types.str;
          default = "";
          description = "Capture only the named menu bar window (optional).";
        };
        options.width = mkOption {
          type = types.int;
          default = 0;
          description = "Rendered width in points (0 = use captured size).";
        };
        options.height = mkOption {
          type = types.int;
          default = 0;
          description = "Rendered height in points (0 = use captured size).";
        };
        options.corner_radius = mkOption {
          type = types.int;
          default = 0;
          description = "Corner radius applied to the captured image.";
        };
        options.update_freq = mkOption {
          type = types.int;
          default = 0;
          description = "Re-capture every N updates; 0 = on event/front-app switch only.";
        };
        options.inverse = mkOption {
          type = types.bool;
          default = false;
          description = "Invert the captured pixels (Difference blend).";
        };
      });
      default = null;
      description = "Alias component (captures another app's menu bar item).";
    };
    options.graph = mkOption {
      type = types.nullOr (types.submodule {
        options.width = mkOption {
          type = types.int;
          default = 0;
          description = "Graph width in points.";
        };
        options.height = mkOption {
          type = types.int;
          default = 0;
          description = "Graph height in points.";
        };
        options.line_width = mkOption {
          type = types.float;
          default = 0.0;
          description = "Graph line stroke width in points (0 = default 1.0).";
        };
        options.fill_color = mkOption {
          type = colorType;
          default = "";
          description = "Area fill color below the line.";
        };
        options.line_color = mkOption {
          type = colorType;
          default = "";
          description = "Line color (default white).";
        };
        options.max_points = mkOption {
          type = types.int;
          default = 0;
          description = "Number of buffered data points shown (0 = default).";
        };
        options.min = mkOption {
          type = types.float;
          default = 0.0;
          description = "Minimum value for the plot range.";
        };
        options.max = mkOption {
          type = types.float;
          default = 0.0;
          description = "Maximum value for the plot range.";
        };
      });
      default = null;
      description = "Graph component (dimensions, colors, data range).";
    };
  });

barItemsType = types.attrsOf barItemType;

  barItemsL = types.submodule {
    options.left = mkOption {
      type = barItemsType;
      default = theme.items.left;
      description = "Bar items shown on the left side.";
    };
    options.right = mkOption {
      type = barItemsType;
      default = theme.items.right;
      description = "Bar items shown on the right side.";
    };
    options.center = mkOption {
      type = barItemsType;
      default = theme.items.center;
      description = "Bar items shown in the center.";
    };
    options.center_left = mkOption {
      type = barItemsType;
      default = theme.items.center_left;
      description = "Bar items shown to the left of center.";
    };
    options.center_right = mkOption {
      type = barItemsType;
      default = theme.items.center_right;
      description = "Bar items shown to the right of center.";
    };
  };
in
{
  options.services.omabar = {
    enable = mkEnableOption "omabar status bar";

    bar = {
      position = mkOption {
        type = types.enum [ "top" "bottom" "left" "right" ];
        default = "top";
        description = "Bar position on screen.";
      };

      height = mkOption {
        type = types.int;
        default = theme.bar.height;
        description = "Bar height in points.";
      };

      width = mkOption {
        type = types.int;
        default = theme.bar.width;
        description = "Bar width in points; 0 = full screen width.";
      };

      margin = mkOption {
        type = types.int;
        default = theme.bar.margin;
        description = "Bar margin from screen edges in points.";
      };

      blur_radius = mkOption {
        type = types.int;
        default = theme.bar.blur_radius;
        description = "Background blur radius in points; 0 = disabled.";
      };

      color = mkOption {
        type = types.str;
        default = theme.bar.color;
        description = "Bar background color (hex, e.g. 0x00000000).";
      };

      corner_radius = mkOption {
        type = types.int;
        default = theme.bar.corner_radius;
        description = "Bar corner radius in points; 0 = square corners.";
      };

      border_width = mkOption {
        type = types.int;
        default = theme.bar.border_width;
        description = "Bar border width in points; 0 = no border.";
      };

      border_color = mkOption {
        type = types.str;
        default = theme.bar.border_color;
        description = "Bar border color (hex, e.g. 0x00000000).";
      };

      shadow = mkOption {
        type = types.bool;
        default = theme.bar.shadow;
        description = "Whether to draw a window shadow under the bar.";
      };

      shadow_color = mkOption {
        type = types.str;
        default = theme.bar.shadow_color;
        description = "Shadow color (hex, e.g. 0x00000000).";
      };

      topmost = mkOption {
        type = types.bool;
        default = true;
        description = "Keep the bar window above other windows.";
      };

      sticky = mkOption {
        type = types.bool;
        default = true;
        description = "Show the bar on all Spaces.";
      };

      notch_width = mkOption {
        type = types.int;
        default = 0;
        description = ''
          Notch width in points; 0 = auto-detect from the display when a
          built-in notch is present, otherwise no notch handling.
        '';
      };

      notch_offset = mkOption {
        type = types.int;
        default = 0;
        description = ''
          Vertical offset (points) pushed below the notch on built-in
          displays when a notch is present.
        '';
      };

      notch_auto_offset = mkOption {
        type = types.bool;
        default = true;
        description = ''
          When true (the default) and `notch_offset` is 0, a top bar on a
          notched display is pushed below the camera housing so a floating
          rounded container is never clipped. Set this to false to keep the bar
          flush at `y_offset` so it spans the native menu bar band instead;
          `notch_width` still keeps centre-positioned items clear of the
          camera housing.
        '';
      };

      y_offset = mkOption {
        type = types.int;
        default = theme.bar.y_offset;
        description = "Vertical offset of the bar in points.";
      };

      alpha = mkOption {
        type = types.float;
        default = 1.0;
        description = "Bar window opacity from 0.0 to 1.0.";
      };
    };

    defaults = {
      icon.font = mkOption {
        type = types.str;
        default = theme.defaults.icon.font;
        description = "Font string for item icons.";
      };

      icon.color = mkOption {
        type = types.str;
        default = theme.defaults.icon.color;
        description = "Icon color (hex, AARRGGBB).";
      };

      icon.highlight_color = mkOption {
        type = types.str;
        default = theme.defaults.icon.highlight_color;
        description = "Highlighted icon color (hex, AARRGGBB).";
      };

      label.font = mkOption {
        type = types.str;
        default = theme.defaults.label.font;
        description = "Font string for item labels.";
      };

      label.color = mkOption {
        type = types.str;
        default = theme.defaults.label.color;
        description = "Label color (hex, AARRGGBB).";
      };

      label.highlight_color = mkOption {
        type = types.str;
        default = theme.defaults.label.highlight_color;
        description = "Highlighted label color (hex, AARRGGBB).";
      };

      background.color = mkOption {
        type = types.str;
        default = theme.defaults.background.color;
        description = "Item background color (hex, AARRGGBB).";
      };

      background.border_color = mkOption {
        type = types.str;
        default = theme.defaults.background.border_color;
        description = "Item background border color (hex, AARRGGBB).";
      };

      background.corner_radius = mkOption {
        type = types.int;
        default = theme.defaults.background.corner_radius;
        description = "Item background corner radius in points.";
      };

      background.border_width = mkOption {
        type = types.int;
        default = theme.defaults.background.border_width;
        description = "Item background border width in points.";
      };

      padding_left = mkOption {
        type = types.int;
        default = theme.defaults.padding_left;
        description = "Default left padding for items in points.";
      };

      padding_right = mkOption {
        type = types.int;
        default = theme.defaults.padding_right;
        description = "Default right padding for items in points.";
      };
    };

    defaultItem = mkOption {
      type = barItemType;
      default = { };
      description = "Template applied to every bar item before per-item options.";
    };

    items = mkOption {
      type = barItemsL;
      default = { };
      description = "Bar items organised into bar sections (left/right/center/…).";
    };

    components = {
      clock = {
        enable = mkEnableOption "clock component (right side)";
        format = mkOption {
          type = types.str;
          default = "%a %d %b %H:%M";
          description = "strftime format string for the clock display.";
        };
        update_interval = mkOption {
          type = types.int;
          default = 60;
          description = "Re-render every N ticks (~1 tick per frame).";
        };
        icon = mkOption {
          type = colorType;
          default = "";
          description = "Clock icon character/string.";
        };
        label_font = mkOption {
          type = types.str;
          default = "";
          description = "Override font for the clock label.";
        };
        label_color = mkOption {
          type = colorType;
          default = "";
          description = "Override color for the clock label.";
        };
      };

      battery = {
        enable = mkEnableOption "battery component (right side)";
        update_interval = mkOption {
          type = types.int;
          default = 0;
          description = "Re-render interval in ticks; 0 = on event only.";
        };
        icon_strip = mkOption {
          type = types.listOf types.str;
          default = [ ];
          description = "Battery icon strip (low to full).";
        };
        label_font = mkOption {
          type = types.str;
          default = "";
          description = "Override font for the battery label.";
        };
        label_color = mkOption {
          type = colorType;
          default = "";
          description = "Override color for the battery label.";
        };
      };

      volume = {
        enable = mkEnableOption "volume component (right side)";
        icon_strip = mkOption {
          type = types.listOf types.str;
          default = [ ];
          description = "Volume icon strip (muted to loud).";
        };
        label_font = mkOption {
          type = types.str;
          default = "";
          description = "Override font for the volume label.";
        };
        label_color = mkOption {
          type = colorType;
          default = "";
          description = "Override color for the volume label.";
        };
      };

      wifi = {
        enable = mkEnableOption "wifi component (right side)";
        icon = mkOption {
          type = types.str;
          default = "";
          description = "WiFi icon character/string.";
        };
        label_font = mkOption {
          type = types.str;
          default = "";
          description = "Override font for the wifi label.";
        };
        label_color = mkOption {
          type = colorType;
          default = "";
          description = "Override color for the wifi label.";
        };
      };

      media = {
        enable = mkEnableOption "media component (right side)";
        label_font = mkOption {
          type = types.str;
          default = "";
          description = "Override font for the media label.";
        };
        label_color = mkOption {
          type = colorType;
          default = "";
          description = "Override color for the media label.";
        };
        truncation = mkOption {
          type = types.int;
          default = 45;
          description = "Truncate media text to this many characters.";
        };
      };

      front_app = {
        enable = mkEnableOption "front app component (center)";
        icon = mkOption {
          type = types.str;
          default = "";
          description = "Front app icon character/string.";
        };
        label_font = mkOption {
          type = types.str;
          default = "";
          description = "Override font for the front app label.";
        };
        label_color = mkOption {
          type = colorType;
          default = "";
          description = "Override color for the front app label.";
        };
      };
    };

    animations = {
      enable = mkEnableOption "bar item animations";
      duration = mkOption {
        type = types.float;
        default = theme.animations.duration;
        description = "Animation duration in seconds.";
      };
      function = mkOption {
        type = types.enum [ "linear" "ease_in" "ease_out" "ease_in_out" "spring" ];
        default = theme.animations.function;
        description = "Easing function used for animation.";
      };
    };

    daemon = {
      hotload = mkEnableOption "config hot-reloading";
      log_level = mkOption {
        type = types.enum [ "debug" "info" "warn" "error" ];
        default = "info";
        description = "Daemon log verbosity.";
      };
      pid_file = mkOption {
        type = types.path;
        default = "/tmp/omabar.pid";
        description = "PID file written by the daemon.";
      };
      lock_file = mkOption {
        type = types.path;
        default = "/tmp/omabar.lock";
        description = "Lock file guarding single-instance runs.";
      };
    };

    fonts = {
      packages = mkOption {
        type = types.listOf types.package;
        default = theme.fonts.packages;
        description = "Nix font packages installed for the bar (installed into /Library/Fonts/Nix Fonts).";
      };
      default = mkOption {
        type = types.str;
        default = theme.fonts.default;
        description = "Default font string (family:style:size) used when items omit it.";
      };
    };

    events = mkOption {
      type = types.attrsOf (types.submodule {
        options.name = mkOption {
          type = types.str;
          description = "Event name used in update_masks and by the daemon.";
        };
        options.notification = mkOption {
          type = types.nullOr types.str;
          default = null;
          description = "Optional NSDistributedNotification name that triggers the event.";
        };
      });
      default = { };
      description = "Custom event registrations expanded into the daemon.";
    };

    plugins = mkOption {
      type = types.attrsOf (types.submodule {
        options.enable = mkEnableOption "plugin";
        options.package = mkOption {
          type = types.package;
          description = "Plugin package to install (built with the plugin SDK).";
        };
        options.update_interval = mkOption {
          type = types.int;
          default = 0;
          description = "Refresh interval in ticks; 0 = on event only.";
        };
        options.env = mkOption {
          type = types.attrsOf types.str;
          default = { };
          description = "Environment variables exported to the plugin process.";
        };
      });
      default = { };
      description = "Swift plugin apps installed and managed by the bar daemon.";
    };

    configFile = mkOption {
      type = types.path;
      readOnly = true;
      description = "Binary config file generated from the module options.";
    };
  };

  config = {
  # Deliberately unconditional. `enable = false` used to mean only "nix-darwin
  # stops managing omabar": the daemon forked, so it outlived the LaunchAgent
  # being deleted, and the installed ~/Applications/Omabar.app was never
  # removed either. This script has to run in both states, so it cannot live
  # behind `mkIf cfg.enable`.
  system.activationScripts.postActivation.text = lib.mkAfter (
    # NOTE: this must be the existing `postActivation` script. nix-darwin only
    # ever runs `preActivation` and `postActivation`; a custom key such as
    # `system.activationScripts.omabarApp` is accepted by the module system,
    # shows up under config.system.activationScripts, and is then silently
    # never executed. That left ~/Applications/Omabar.app missing while the
    # LaunchAgent pointed straight at it, so the bar could not start at all.
    (lib.optionalString (cfg.enable && primaryUser != null) ''
      target="${omabarApp}"
      source="${pkgs.omabar}/Applications/Omabar.app"
      uid=$(/usr/bin/id -u ${primaryUser})

      if [ -d "$source" ]; then
        mkdir -p "$(dirname "$target")"
        # Replace atomically-ish: build the new copy alongside, then swap, so
        # a running daemon is never left with a half-written bundle.
        rm -rf "$target.new"
        mkdir -p "$target.new"
        cp -R "$source/." "$target.new/"
        rm -rf "$target"
        mv "$target.new" "$target"

        if [ ! -x "$target/Contents/MacOS/Omabar" ]; then
          echo "omabar: installed bundle is missing its executable" >&2
          exit 1
        fi
      else
        # A caller overrode `package` with a derivation that ships no bundle.
        # Fall back to the bare binary so the bar still runs; it just will not
        # be clickable, because a cdhash identity cannot keep a grant.
        mkdir -p "$(dirname "$target")/Omabar.app/Contents/MacOS"
        cp -R "${pkgs.omabar}/bin/omabar" \
          "$target/Contents/MacOS/Omabar" 2>/dev/null || true
        echo "omabar: WARNING - ${pkgs.omabar} ships no signed bundle;" >&2
        echo "omabar: WARNING - clicks will break on every rebuild." >&2
      fi

      # Make sure the agent is actually running now that the bundle is in place.
      #
      # Two distinct ways it can fail to be running, both invisible unless
      # checked for:
      #
      # 1. Never started. setupLaunchAgents runs *before* this postActivation
      #    script, so on a re-enable the agent is launched while
      #    ~/Applications/Omabar.app is still the version enable=false deleted.
      #    The launch fails on a missing binary and, KeepAlive being false,
      #    nothing retries — leaving a correctly installed bundle with no bar.
      #
      # 2. Blocked by an orphan. A generation predating --foreground forked,
      #    and that orphan keeps the daemon lockfile forever: the agent starts,
      #    fails to take the lock, exits 1, and again nothing retries. This is
      #    the nastier one, because launchd then reports the job as "not
      #    running, last exit code 1" while the old bar is still on screen
      #    still rendering the OLD config — so a geometry or feature change
      #    looks like the rebuild did nothing. I hit exactly this: set the bar
      #    into the menu bar band, rebuilt, and measured the pre-rebuild PID
      #    still drawing the old 44pt floating bar. Reading that process's
      #    --config showed the stale store path, which is what identified it.
      #
      # Only acts when launchd has no live pid for the job, and a tracked
      # instance always has one, so a healthy bar is never restarted and
      # ordinary rebuilds are unaffected.
      job_pid=$(/bin/launchctl print "gui/$uid/org.nixos.omabar" 2>/dev/null \
        | sed -n 's/^[[:space:]]*pid = \([0-9][0-9]*\).*/\1/p' | head -1)
      if [ -z "$job_pid" ]; then
        # An instance the job does not track would keep winning the lockfile,
        # so clear it before asking launchd to start a fresh one.
        if /usr/bin/pgrep -x Omabar >/dev/null 2>&1; then
          echo "omabar: untracked instance holds the lock; restarting the agent" >&2
          /usr/bin/pkill -x Omabar 2>/dev/null || true
          sleep 1
        fi
        /bin/launchctl kickstart -k "gui/$uid/org.nixos.omabar" 2>/dev/null || true
      fi
    '') +

    # Disabling must actually remove omabar, not just stop managing it. By
    # the time postActivation runs, nix-darwin has already dropped
    # org.nixos.omabar from ~/Library/LaunchAgents, and the agent now runs in
    # the foreground so launchd sends SIGTERM and omabar exits via its own
    # handler. The pkill is a belt-and-braces sweep for orphans spawned by an
    # older generation that still forked: those are invisible to launchd, so
    # nothing else would ever reap them. Absolute paths because the activation
    # environment has a near-empty PATH.
    (lib.optionalString (!cfg.enable && primaryUser != null) ''
      target="${omabarApp}"
      uid=$(/usr/bin/id -u ${primaryUser})

      /bin/launchctl bootout "gui/$uid/org.nixos.omabar" 2>/dev/null || true
      /usr/bin/pkill -x Omabar 2>/dev/null || true

      if [ -e "$target" ]; then
        rm -rf "$target"
        echo "omabar: disabled, removed $target"
      fi
    '')
  );
    # The options below only mean anything while omabar is enabled, so they are
    # guarded one attribute at a time instead of by wrapping the whole body in
    # `mkIf cfg.enable`. Merging an unconditional attrset with an mkIf one --
    # `{ a = ...; } // mkIf cond { b = ...; }` -- copies `_type = "if"` onto
    # the merged result, and the module system then treats the *entire* config
    # as conditional. That silently discards the install/cleanup script above
    # whenever omabar is disabled, which is precisely when cleanup must run.
    services.omabar.configFile = lib.mkIf cfg.enable (
      configGenerator.generate { cfg = cfg; }
    );

    fonts.packages = lib.mkIf cfg.enable cfg.fonts.packages;

    # Stable symlink for the config file — on nix-darwin reconfigure the
    # store path changes, but the /etc entry keeps the same path.  When
    # hotload is enabled the daemon's in-process FSEvents watcher and the
    # launchd WatchPaths both monitor this stable path.
    environment.etc."omabar_config".source = lib.mkIf cfg.enable cfg.configFile;

    # Install the signed bundle at a stable path the user can actually grant
    # Accessibility to, and run the daemon from that copy.
    #
    # The Accessibility grant is the one thing about this bar that macOS will
    # not let us configure: it has to be toggled by hand in System Settings.
    # That is only workable if the thing being granted has a path that (a) the
    # System Settings file picker can reach and (b) does not change when
    # omabar is rebuilt. /nix/store fails (a) on both counts -- the picker hides
    # it and the "+" button rejects store paths -- and the store path changes
    # on every rebuild, failing (b) as well. Copying the signed bundle to
    # ~/Applications fixes both. The copy keeps its build-independent signing
    # identity, so the grant made against it keeps working across rebuilds.
    # NOTE: this must go in the existing `postActivation` script. nix-darwin
    # only ever runs `preActivation` and `postActivation`; a custom key such as
    # `system.activationScripts.omabarApp` is accepted by the module system,
    # shows up under config.system.activationScripts, and is then silently never
    # executed. That left ~/Applications/Omabar.app missing while the
    # LaunchAgent pointed straight at it, so the bar could not start at all.

    # The bar is a menu-bar app: it needs the user's GUI session (WindowServer,
    # CoreVideo, CFPreferences) and it refuses to run as root. nix-darwin's
    # `launchd.agents` land in /Library/LaunchAgents, which the activation
    # loads with `launchctl load` in the *system* domain, so an agent with no
    # UserName is launched as root and dies immediately with
    # "omabar: refusing to run as root" — which surfaces only as a launchd job
    # stuck at "not running, last exit code 1" with nothing in the log.
    #
    # `environment.userLaunchAgents` is the mechanism meant for this: the plist
    # is copied into the primary user's ~/Library/LaunchAgents and loaded with
    # `launchctl asuser <uid> sudo --user=<user> launchctl load`, which puts it
    # in that user's GUI session.
    environment.userLaunchAgents = lib.mkIf (cfg.enable && primaryUser != null) {
      omabar.source = pkgs.writeText "org.nixos.omabar.plist" ''
        <?xml version="1.0" encoding="UTF-8"?>
        <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
        <plist version="1.0">
        <dict>
          <key>Label</key>
          <string>org.nixos.omabar</string>
          <key>ProgramArguments</key>
          <array>
            <string>/bin/sh</string>
            <string>-c</string>
            <!-- The config path must be passed as --config: omabar only ever
                 reads it from argv (src/main.c), never from the environment.
                 getenv() is called solely for HOME and USER, so setting
                 OMABAR_CONFIG_FILE here silently did nothing and the daemon
                 booted on its built-in defaults -- margin 0, height 40, no
                 corner radius, no border -- which is why the bar rendered as
                 a full-bleed dark strip instead of a floating pill. -->
            <!-- Run the signed .app bundle's binary, not $out/bin/omabar.
                 Clicks need a CGEventTap, which macOS gates behind
                 Accessibility, and TCC keys that grant to the code signing
                 identity. The bare binary is ad-hoc signed, so its identity is
                 a cdhash of its exact bytes: every rebuild produced a new
                 hash and silently revoked the previous grant, leaving the bar
                 permanently unclickable. The bundle is signed with a fixed
                 `identifier "com.nowarelabs.omabar"` requirement, so one grant
                 survives rebuilds. Falls back to the bare binary if a caller
                 overrides the package with one that has no bundle. -->
            <!-- --foreground keeps the process in the foreground so launchd
                 supervises the real PID instead of a short-lived parent that
                 _exit(0)s (daemonize() in src/main.c). Without it, launchd
                 saw the job finish on the parent's exit, and the forked child
                 was an orphan launchd did not track: unloading or deleting the
                 plist could not stop it, so `enable = false` left the bar on
                 screen until something killed it by hand. src/main.c documents
                 why the fork exists at all — the child, not a fork-surviving
                 parent, must perform the CoreFoundation/CoreVideo bootstrap —
                 and --foreground satisfies that equally well, because there is
                 no fork: this process *is* the one that calls omabar_init(). -->
            <string>/bin/wait4path /nix/store &amp;&amp; exec ${lib.escapeShellArg omabarBin} --foreground --config ${lib.escapeShellArg (if cfg.daemon.hotload then "/etc/omabar_config" else toString cfg.configFile)}</string>
          </array>
          <key>EnvironmentVariables</key>
          <dict>
            <!-- fork_exec() runs "/usr/bin/env sh -c <script>", so both `sh`
                 and the commands in a click_script are resolved through PATH.
                 A nix-only PATH left sh, open and osascript unresolvable, so
                 every click script failed silently. Keep the system paths
                 launchd would normally supply. -->
            <key>PATH</key>
            <string>${lib.makeBinPath [ pkgs.coreutils pkgs.findutils ]}:${pkgs.omabar}/bin:/usr/bin:/bin:/usr/sbin:/sbin</string>
          </dict>
          <key>RunAtLoad</key>
          <true/>
          <key>KeepAlive</key>
          <false/>
          <key>ProcessType</key>
          <string>Interactive</string>
          <key>ThrottleInterval</key>
          <integer>1</integer>
        ${lib.optionalString cfg.daemon.hotload ''
          <key>WatchPaths</key>
          <array>
            <string>/etc/omabar_config</string>
          </array>''}
        </dict>
        </plist>
      '';
      omabar.target = "org.nixos.omabar.plist";
    };
  };

}