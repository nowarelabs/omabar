/*
 * nix/default-theme.nix — Omabar's opinionated default dark theme.
 *
 * The "out of the box" look: a floating rounded pill of deep violet glass
 * hovering over the desktop, inset from the screen edges and dropped below
 * the camera housing on notched MacBooks. Inside it, spaced like a status
 * bar rather than a menu bar:
 *
 *   left   workspace pills (one per Space, the active one solid) · hairline
 *          · now-playing note + artist/title
 *   right  battery % · volume % · 24-hour date and time
 *
 * Icons are drawn as vectors by the bar itself, so this theme needs no icon
 * font to look right. The attribute set is consumed by module.nix as the
 * *default value* for the corresponding `services.omabar` options, so a user
 * who configures nothing gets this theme and can override any single field.
 */
{
  bar = {
    position = "top";
    height = 44;
    width = 0;          /* 0 = span the display, minus the margins */
    margin = 12;        /* horizontal inset from the screen edges */

    /* the floating container */
    corner_radius = 12;
    border_width = 1;
    border_color = "0x24ffffff";
    blur_radius = 40;
    color = "0x8f2b1f4a";  /* deep violet glass, ~56% opaque */
    shadow = true;
    shadow_color = "0x73000000";

    topmost = true;
    sticky = true;

    /* 0 lets the bar read the display's own safe-area inset, so a top bar
       clears the notch instead of being clipped by it. Set an explicit
       value to override. */
    notch_width = 0;
    notch_offset = 0;
    y_offset = 8;

    /* window opacity, not background opacity: the glass comes from
       bar.color's alpha, and dimming the window would fade the text too. */
    alpha = 1.0;
  };

  defaults = {
    icon = {
      font = "SF Pro:Regular:13.0";
      color = "0xfff2f4f8";
      highlight_color = "0xff1b1430";
      string = "";
    };
    label = {
      font = "SF Pro:Regular:13.0";
      color = "0xfff2f4f8";
      highlight_color = "0xff1b1430";
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
      /* One pill per Space. The active one is a solid light chip with dark
         semibold numerals; the rest sit on a faint wash. Numbers come from
         the chip's position, so the strip reads 1, 2, 3, 4 rather than
         depending on an icon font. */
      spaces = {
        order = 10;          # workspaces
        type = "space";
        position = "l";
        space_gap = 4;
        icon = {
          font = "SF Pro:Semibold:12.0";
          color = "0xb2ffffff";
          highlight_color = "0xff1b1430";
        };
        background = {
          color = "0x1cffffff";
          corner_radius = 8;
          border_width = 0;
        };
        selected_background = {
          color = "0xfff4f2fb";
          corner_radius = 8;
          border_width = 0;
        };
        padding_left = 9;
        padding_right = 9;
        update_mask = [ "space_changed" "display_changed" ];
      };

      /* separator between the workspace strip and now playing */
      divider = {
        order = 20;          # separator
        type = "divider";
        position = "l";
        divider_width = 1;
        divider_color = "0x2effffff";
        padding_left = 12;
        padding_right = 12;
      };

      media = {
        order = 30;          # now playing
        type = "media";
        position = "l";
        label = {
          font = "SF Pro:Medium:12.0";
          color = "0xdcece4f7";
        };
        padding_left = 2;
        padding_right = 4;
        update_interval = 60;
        update_mask = [ "media_changed" "front_app_switched" "scroll.tick" ];
      };
    };

    center = { };

    center_left = { };

    right = {
      /* No icons here beyond the bar's own vector glyphs, so the metrics
         read as a calm group of numbers. */
      battery = {
        order = 10;          # battery
        type = "battery";
        position = "r";
        label = {
          font = "SF Pro:Medium:12.0";
          color = "0xfff2f4f8";
        };
        padding_left = 4;
        padding_right = 12;
        update_mask = [ "power_changed" ];
        # no OMABC key for popup contents yet, so give the pill a real action
        click_script = "open x-apple.systempreferences:com.apple.Battery-Settings.extension";
      };

      volume = {
        order = 20;          # volume
        type = "volume";
        position = "r";
        label = {
          font = "SF Pro:Medium:12.0";
          color = "0xfff2f4f8";
        };
        padding_left = 4;
        padding_right = 12;
        update_mask = [ "volume_changed" "mute_changed" ];
        # mute toggle, the usual bar behaviour for a volume pill
        click_script = "osascript -e 'set v to get volume settings' -e 'set volume output muted not (output muted of v)'";
      };

      clock = {
        order = 30;          # clock
        type = "clock";
        position = "r";
        format = "%d/%m %H:%M";
        label = {
          font = "SF Mono:Medium:12.0";
          color = "0xfff2f4f8";
        };
        padding_left = 2;
        padding_right = 12;
        update_interval = 60;
        update_mask = [ "scroll.tick" ];
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
