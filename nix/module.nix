{ config, lib, pkgs, ... }:

with lib;

let
  cfg = config.services.omabar;

  configGenerator = import ./config-generator.nix { inherit lib pkgs; };

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
      type = types.enum [ "l" "r" "c" "q" "e" ];
      default = "l";
      description = "Bar section: l=left, r=right, c=center, q=quarter, e=edge.";
    };
    options.icon = mkOption {
      type = fontType;
      default = { };
      description = "Item icon (defaults or overrides per item).";
    };
    options.label = mkOption {
      type = textType;
      default = { };
      description = "Item label (defaults or overrides per item).";
    };
    options.background = mkOption {
      type = backgroundType;
      default = { };
      description = "Item background (defaults or overrides per item).";
    };
    options.update_interval = mkOption {
      type = types.int;
      default = 0;
      description = "Re-run the script every N scroll ticks; 0 = on event only.";
    };
    options.update_mask = mkOption {
      type = types.listOf types.str;
      default = [ ];
      description = "Event names that trigger this item's script.";
    };
    options.associated_space = mkOption {
      type = types.int;
      default = -1;
      description = "Only show on this Space (-1 = all).";
    };
    options.associated_display = mkOption {
      type = types.int;
      default = -1;
      description = "Only show on this display (-1 = all).";
    };
    options.script = mkOption {
      type = types.str;
      default = "";
      description = "ScriptPath or shell command to produce the item content.";
    };
    options.click_script = mkOption {
      type = types.str;
      default = "";
      description = "Script command run on click.";
    };
    options.y_offset = mkOption {
      type = types.int;
      default = 0;
      description = "Vertical offset of the item in points.";
    };
    options.padding_left = mkOption {
      type = types.int;
      default = 2;
      description = "Left padding in points.";
    };
    options.padding_right = mkOption {
      type = types.int;
      default = 2;
      description = "Right padding in points.";
    };
    options.label_x_offset = mkOption {
      type = types.int;
      default = 0;
      description = "Horizontal offset of the label in points.";
    };
    options.icon_x_offset = mkOption {
      type = types.int;
      default = 0;
      description = "Horizontal offset of the icon in points.";
    };
    options.scroll_enabled = mkOption {
      type = types.bool;
      default = false;
      description = "Whether the item reacts to scroll events.";
    };
    options.click_enabled = mkOption {
      type = types.bool;
      default = true;
      description = "Whether the item reacts to click events.";
    };
    options.mach_helper = mkOption {
      type = types.nullOr types.str;
      default = null;
      description = "Mach helper (background light) to install.";
    };
    options.type = mkOption {
      type = types.str;
      default = "";
      description = "Component type; empty = plain script item, space = space component.";
    };
    options.icon_strip = mkOption {
      type = types.listOf types.str;
      default = [ ];
      description = "Icon strip for the space component (per-space icons).";
    };
    options.icon_highlight_color = mkOption {
      type = colorType;
      default = "";
      description = "Highlighted icon color used for selected spaces.";
    };
    options.selected_background = mkOption {
      type = types.submodule {
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
      };
      default = { };
      description = "Background used for the selected space.";
    };
  });

barItemsType = types.attrsOf barItemType;

  barItemsL = types.submodule {
    options.left = mkOption {
      type = barItemsType;
      default = { };
      description = "Bar items shown on the left side.";
    };
    options.right = mkOption {
      type = barItemsType;
      default = { };
      description = "Bar items shown on the right side.";
    };
    options.center = mkOption {
      type = barItemsType;
      default = { };
      description = "Bar items shown in the center.";
    };
    options.center_left = mkOption {
      type = barItemsType;
      default = { };
      description = "Bar items shown to the left of center.";
    };
    options.center_right = mkOption {
      type = barItemsType;
      default = { };
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
        default = 40;
        description = "Bar height in points.";
      };

      width = mkOption {
        type = types.int;
        default = 0;
        description = "Bar width in points; 0 = full screen width.";
      };

      margin = mkOption {
        type = types.int;
        default = 0;
        description = "Bar margin from screen edges in points.";
      };

      blur_radius = mkOption {
        type = types.int;
        default = 0;
        description = "Background blur radius in points; 0 = disabled.";
      };

      color = mkOption {
        type = types.str;
        default = "";
        description = "Bar background color (hex, e.g. 0x00000000).";
      };

      shadow = mkOption {
        type = types.bool;
        default = false;
        description = "Whether to draw a window shadow under the bar.";
      };

      shadow_color = mkOption {
        type = types.str;
        default = "";
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

      y_offset = mkOption {
        type = types.int;
        default = 0;
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
        default = "JetBrainsMono Nerd Font:style=Regular:size=13.0";
        description = "Font string for item icons.";
      };

      icon.color = mkOption {
        type = types.str;
        default = "0xffffffff";
        description = "Icon color (hex, AARRGGBB).";
      };

      icon.highlight_color = mkOption {
        type = types.str;
        default = "0xffffffff";
        description = "Highlighted icon color (hex, AARRGGBB).";
      };

      label.font = mkOption {
        type = types.str;
        default = "Hack Nerd Font:style=Medium:size=13.0";
        description = "Font string for item labels.";
      };

      label.color = mkOption {
        type = types.str;
        default = "0xffffffff";
        description = "Label color (hex, AARRGGBB).";
      };

      label.highlight_color = mkOption {
        type = types.str;
        default = "0xffffffff";
        description = "Highlighted label color (hex, AARRGGBB).";
      };

      background.color = mkOption {
        type = types.str;
        default = "0x00000000";
        description = "Item background color (hex, AARRGGBB).";
      };

      background.border_color = mkOption {
        type = types.str;
        default = "0xffffffff";
        description = "Item background border color (hex, AARRGGBB).";
      };

      background.corner_radius = mkOption {
        type = types.int;
        default = 0;
        description = "Item background corner radius in points.";
      };

      background.border_width = mkOption {
        type = types.int;
        default = 0;
        description = "Item background border width in points.";
      };

      padding_left = mkOption {
        type = types.int;
        default = 2;
        description = "Default left padding for items in points.";
      };

      padding_right = mkOption {
        type = types.int;
        default = 2;
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
        default = 0.2;
        description = "Animation duration in seconds.";
      };
      function = mkOption {
        type = types.enum [ "linear" "ease_in" "ease_out" "ease_in_out" "spring" ];
        default = "ease_in_out";
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
        default = [ ];
        description = "Nix font packages installed for the bar (installed into /Library/Fonts/Nix Fonts).";
      };
      default = mkOption {
        type = types.str;
        default = "";
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

  config = mkIf cfg.enable {
    services.omabar.configFile = configGenerator.generate { cfg = cfg; };

    fonts.packages = cfg.fonts.packages;

    # Stable symlink for the config file — on nix-darwin reconfigure the
    # store path changes, but the /etc entry keeps the same path.  When
    # hotload is enabled the daemon's in-process FSEvents watcher and the
    # launchd WatchPaths both monitor this stable path.
    environment.etc."omabar_config".source = cfg.configFile;

    launchd.agents.omabar = {
      command = "${pkgs.omabar}/bin/omabar";
      path = [ pkgs.coreutils pkgs.findutils "${pkgs.omabar}/bin" ];
      environment = {
        OMABAR_CONFIG_FILE =
          if cfg.daemon.hotload
          then "/etc/omabar_config"
          else toString cfg.configFile;
        OMABAR_DEFAULT_FONT = cfg.fonts.default;
      };
      serviceConfig = {
        RunAtLoad = true;
        KeepAlive = false;
        ProcessType = "UserInteractive";
        ThrottleInterval = 1;
      } // (lib.optionalAttrs cfg.daemon.hotload {
        WatchPaths = [ "/etc/omabar_config" ];
      });
    };
  };
}