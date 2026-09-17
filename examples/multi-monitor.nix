# Multi-display omabar configuration. Items are scoped per display with
# `associated_display` so each screen shows different content (e.g. clock
# and spaces on the secondary display only).

{ ... }:
{
  services.omabar = {
    enable = true;

    items = {
      # Left on every display: spaces.
      left.spaces = {
        type = "space";
        icon_strip = [ "一" "二" "三" "四" "五" "六" "七" "八" "九" "十" ];
        icon_highlight_color = "0xff40a0ff";
      };

      # Clock only on the built-in display.
      right.clock_main = {
        type = "clock";
        associated_display = 1;        # display 1 (CGDirectDisplayID order)
        label.font = "SF Mono:Regular:12.0";
      };

      # Media info only on the external display.
      right.media_ext = {
        type = "media";
        associated_display = 2;
      };

      # Front-app label on every display but scoped to its own space.
      center_left.front_app = {
        type = "front_app";
        associated_space = -1;         # -1 = all Spaces
      };
    };
  };
}