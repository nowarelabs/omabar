# Full omabar configuration exercising the option surface: custom bar
# styling, per-item fonts/colors, an alias item, a graph item, a custom
# script item, a component, custom events and a plugin.

{ config, lib, pkgs, ... }:
{
  services.omabar = {
    enable = true;

    bar = {
      position = "top";
      height = 40;
      margin = 8;
      blur_radius = 24;
      color = "0xcc1e1e2e";           # Catppuccin Mocha base, ~80% opaque
      shadow = true;
      shadow_color = "0x88000000";
      topmost = true;
      sticky = true;
      y_offset = 0;
      alpha = 1.0;
    };

    defaults.icon = {
      font = "Iosevka:Regular:13.0";
      color = "0xffcdd6f4";
      highlight_color = "0xff89b4fa";
    };
    defaults.label = {
      font = "Iosevka:Regular:13.0";
      color = "0xffcdd6f4";
    };
    defaults.background = {
      color = "0x00000000";
      corner_radius = 8;
    };
    defaults.padding_left = 7;
    defaults.padding_right = 7;

    # Components are one-line shortcuts (components.clock.enable = true;).
    # The default theme already ships clock / battery / wifi / media / front_app
    # as items, so enabling components here would render them twice. To use the
    # shortcuts instead, clear the section: items.right = { }; first.
    # components = {
    #   clock.enable = true;
    #   battery.enable = true;
    # };

    fonts.packages = [ (pkgs.nerd-fonts.iosevka) ];
    fonts.default = "Iosevka:Regular:13.0";

    animations = {
      enable = true;
      duration = 0.2;
      function = "ease_out";
    };

    items = {
      # Left: spaces with a Nerd Font strip + active whitelist.
      left.spaces = {
        type = "space";
        icon_strip = [
          "一" "二" "三" "四" "五"
        ];
        icon_highlight_color = "0xff89b4fa";
        background.color = "0x33ffffff";
        background.corner_radius = 8;
        selected_background.color = "0xff89b4fa";
        selected_background.corner_radius = 8;
        padding_left = 5;
        padding_right = 5;
      };

      # Center-left: front-app label, keep the app's icon via alias.
      center_left.front_app = {
        type = "front_app";
        alias = {
          bundle_id = "";
          update_freq = 1;             # refresh on front-app switch + ticks
        };
        label = {
          font = "Iosevka:Semibold:13.0";
          color = "0xffffffff";
        };
        padding_left = 10;
        padding_right = 10;
      };

      # Right: CPU graph fed by a plugin, then clock on the extreme right.
      right.cpu = {
        type = "";
        icon = {
          string = "cpu";
          font = "Iosevka:Regular:12.0";
          color = "0xffa6e3a1";
        };
        graph = {
          width = 56;
          height = 30;
          line_width = 1.5;
          line_color = "0xffa6e3a1";
          fill_color = "0x22a6e3a1";
          max_points = 80;
          min = 0.0;
          max = 1.0;
        };
        update_mask = [ "scroll.tick" ];
      };

      right.clock = {
        type = "clock";
        label = {
          font = "Iosevka:Mono:12.0";
          color = "0xffa6adc8";
        };
        padding_left = 8;
        padding_right = 8;
      };
    };

    daemon = {
      log_level = "debug";
      hotload = true;                  # relaunch on /etc/omabar_config change
    };

    # Reuse the daemon's automation space: webpack's Menu Bar toggle.
    events.toggle_bar = {
      name = "toggle_bar";
      notification = "com.omabar.toggle";
    };

    # A plugin built with the plugin SDK (see nix/plugin-sdk.nix). This toy
    # package just installs a shell script; swap for your own derivation.
    plugins.cpu = {
      enable = true;
      package = pkgs.runCommand "cpu-plugin" { } ''
        mkdir -p $out/bin
        cat > $out/bin/cpu-plugin <<'EOF'
        #!/bin/sh
        # print CPU load as a 0..1 number for the graph item
        top -l 1 | awk '/CPU usage/ { printf "%0.2f", $3/100 }'
        EOF
        chmod +x $out/bin/cpu-plugin
      '';
      update_interval = 5;
      env.OMABAR_PUSH = "graph";
    };
  };
}