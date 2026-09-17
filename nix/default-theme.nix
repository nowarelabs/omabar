/*
 * nix/default-theme.nix — Omabar's opinionated default dark theme.
 *
 * This is the "out of the box" look: translucent dark bar with a soft blur,
 * SF Pro text, an accent blue for selections, spaces on the left, front-app on
 * the center-left, and clock / volume / battery / wifi / media on the right.
 *
 * The attribute set is consumed by module.nix as the *default value* for the
 * corresponding `services.omabar` options, so a user who configures nothing
 * gets this theme and can override any single field via Nix.
 */
{
  bar = {
    position = "top";
    height = 38;
    width = 0;
    margin = 0;
    blur_radius = 30;
    color = "0x40000000";
    shadow = true;
    shadow_color = "0x40000000";
    topmost = true;
    sticky = true;
    notch_width = 0;
    notch_offset = 0;
    y_offset = 0;
    alpha = 1.0;
  };

  defaults = {
    icon = {
      font = "SF Pro:Regular:13.0";
      color = "0xfff2f4f8";
      highlight_color = "0xff40a0ff";
      string = "";
    };
    label = {
      font = "SF Pro:Regular:13.0";
      color = "0xfff2f4f8";
      highlight_color = "0xff40a0ff";
      string = "";
    };
    background = {
      color = "0x00000000";
      border_color = "0x33ffffff";
      corner_radius = 6;
      border_width = 0;
    };
    padding_left = 6;
    padding_right = 6;
  };

  items = {
    left = {
      spaces = {
        type = "space";
        position = "l";
        icon_strip = [ "一" "二" "三" "四" "五" "六" "七" "八" "九" "十" ];
        icon_highlight_color = "0xff40a0ff";
        background = {
          color = "0x26ffffff";
          corner_radius = 6;
          border_width = 0;
        };
        selected_background = {
          color = "0xff40a0ff";
          corner_radius = 6;
          border_width = 0;
        };
        padding_left = 5;
        padding_right = 5;
        update_mask = [ "space_changed" "display_changed" ];
      };
    };

    center = { };

    center_left = {
      front_app = {
        type = "front_app";
        position = "q";
        icon = {
          string = "";
        };
        label = {
          font = "SF Pro:Semibold:13.0";
          color = "0xfff2f4f8";
        };
        padding_left = 10;
        padding_right = 10;
        update_mask = [ "front_app_switched" ];
      };
    };

    right = {
      clock = {
        type = "clock";
        position = "r";
        label = {
          font = "SF Mono:Regular:12.0";
          color = "0xffc7cdd9";
        };
        update_interval = 1;
        update_mask = [ "scroll.tick" ];
        padding_left = 8;
        padding_right = 8;
      };

      volume = {
        type = "volume";
        position = "r";
        icon_strip = [ "󰕿" "󰖀" "󰕾" ];
        icon = {
          font = "SF Pro:Regular:13.0";
        };
        padding_left = 5;
        padding_right = 5;
        update_mask = [ "volume_changed" "mute_changed" ];
      };

      battery = {
        type = "battery";
        position = "r";
        icon_strip = [
          "󰂎" "󰁺" "󰁻" "󰁼" "󰁽" "󰁾" "󰁿" "󰂀" "󰂁" "󰂂" "󰁹"
        ];
        padding_left = 5;
        padding_right = 5;
        update_mask = [ "power_changed" ];
      };

      wifi = {
        type = "wifi";
        position = "r";
        icon = {
          string = "󰤨";
        };
        padding_left = 6;
        padding_right = 6;
        update_mask = [ "wifi_changed" ];
      };

      media = {
        type = "media";
        position = "r";
        icon = {
          string = "󰎆";
        };
        label = {
          font = "SF Pro:Regular:12.0";
          color = "0xffc7cdd9";
        };
        padding_left = 6;
        padding_right = 6;
        update_mask = [ "media_changed" "front_app_switched" ];
      };
    };

    center_right = { };
  };

  animations = {
    enable = true;
    duration = 0.25;
    function = "ease_out";
  };

  fonts = {
    default = "SF Pro:Regular:13.0";
    packages = [ ];
  };
}