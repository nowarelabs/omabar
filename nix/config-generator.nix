/*
 * omabar Nix config generator.
 *
 * Takes the evaluated services.omabar module configuration and produces a
 * binary config file (NOT a shell script) that the daemon reads at startup.
 *
 * File layout (flat binary):
 *   bytes 0..4   magic "OMABC"
 *   byte  5      format version (0x01)
 *   bytes 6..15  reserved (zeros)
 *   bytes 16..   JSON blob: { bar, defaults, items, animations, daemon,
 *                            events, plugins }
 *
 * The header is literal binary; everything after it is the JSON payload.
 * A C reader trivially `strncmp`s the magic, then parses the trailing JSON.
 */
{ lib, pkgs }:

let
  inherit (builtins) toJSON attrNames map listToAttrs length isAttrs isList mapAttrs hasAttr;

  inherit (lib) filterAttrs mapAttrsToList groupBy concatLists concatMap filter unique recursiveUpdate;

  theme = import ./default-theme.nix;

  /* Recursively strip null leaves. barItemType options default to null for
     "unset" so that a user override does not clobber the theme's per-item
     defaults with empty typed values. */
  dropNulls = v:
    if isList v then map dropNulls v
    else if isAttrs v then filterAttrs (_: vv: vv != null) (mapAttrs (_: dropNulls) v)
    else v;

  /* Merge order (later wins): theme per-item template <- defaultItem <- user.
     Only the fields the user actually set (non-null) override the theme. */
  mergedItem = cfg: section: name: it:
    recursiveUpdate
      (recursiveUpdate (theme.items.${section}.${name} or { }) (dropNulls (cfg.defaultItem or { })))
      (dropNulls it);

  /* Per-component defaults for the convenience shortcuts (task 65). */
  componentSide = {
    clock     = "right";
    battery   = "right";
    volume    = "right";
    wifi      = "right";
    media     = "right";
    front_app = "center";
  };

  /* Which events each component subscribes to (task 72 refines this). */
  componentEvents = {
    clock     = [ "tick" ];
    battery   = [ "power_changed" ];
    volume    = [ "volume_changed" "mute_changed" ];
    wifi      = [ "wifi_changed" ];
    media     = [ "media_changed" "front_app_changed" ];
    front_app = [ "front_app_changed" ];
    space     = [ "space_changed" "display_changed" ];
  };

  /* Drop non-configurable/no-op fields to keep the blob minimal. */
  cleanIcon = c:
    filterAttrs (n: v: v != "" && v != null)
      { font = c.font or ""; color = c.color or ""; highlight_color = c.highlight_color or ""; };

  cleanLabel = l:
    filterAttrs (n: v: v != "" && v != null)
      { font = l.font or ""; color = l.color or ""; };

  cleanBackground = b:
    filterAttrs (n: v: v != "" && v != 0 && v != null)
      { color = b.color or ""; border_color = b.border_color or ""; corner_radius = b.corner_radius or 0; border_width = b.border_width or 0; };

  cleanAlias = a:
    filterAttrs (n: v: v != "" && v != 0 && v != null && v != false)
      { target_pid = a.target_pid or ""; bundle_id = a.bundle_id or ""; owner = a.owner or ""; name = a.name or "";
        width = a.width or 0; height = a.height or 0; corner_radius = a.corner_radius or 0;
        update_freq = a.update_freq or 0; inverse = a.inverse or false; };

  cleanGraph = g:
    filterAttrs (n: v: v != "" && v != 0 && v != null && v != false)
      { width = g.width or 0; height = g.height or 0; line_width = g.line_width or 0.0;
        fill_color = g.fill_color or ""; line_color = g.line_color or "";
        max_points = g.max_points or 0; min = g.min or 0.0; max = g.max or 0.0; };

  itemToJSON = cfg: section: name: raw:
    let
      it = mergedItem cfg section name raw;
      icon = cleanIcon (it.icon or { });
      label = cleanLabel (it.label or { });
      background = cleanBackground (it.background or { });
      selected = cleanBackground (it.selected_background or { });
      alias = cleanAlias (it.alias or { });
      graph = cleanGraph (it.graph or { });
      opts = it.options or it;
    in
    filterAttrs (n: v: v != "" && v != null && v != { } && v != [ ] && v != false)
      {
        inherit name;
        position = opts.position or "right";
        type = opts.type or "";
        icon = if icon == { } then null else icon;
        icon_strip = if (opts.icon_strip or [ ]) == [ ] then null else opts.icon_strip;
        icon_highlight_color = opts.icon_highlight_color or "";
        label = if label == { } then null else label;
        background = if background == { } then null else background;
        selected_background = if selected == { } then null else selected;
        alias = if alias == { } then null else alias;
        graph = if graph == { } then null else graph;
        update_interval = opts.update_interval or 0;
        update_mask = opts.update_mask or [ ];
        associated_space = opts.associated_space or (-1);
        associated_display = opts.associated_display or (-1);
        script = opts.script or "";
        click_script = opts.click_script or "";
        y_offset = opts.y_offset or 0;
        padding_left = opts.padding_left or 0;
        padding_right = opts.padding_right or 0;
        label_x_offset = opts.label_x_offset or 0;
        icon_x_offset = opts.icon_x_offset or 0;
        scroll_enabled = opts.scroll_enabled or false;
        click_enabled = opts.click_enabled or true;
        mach_helper = opts.mach_helper or null;
      };

  /* Expand enabled components (task 65) into item definitions. */
  componentsToItems = components:
    let
      enabled = filterAttrs (_: c: c.enable or false) components;
      mk = name: c: {
        inherit name;
        type = name;
        icon.string = c.icon or "";
        icon_strip = c.icon_strip or [ ];
        label.font = c.label_font or "";
        label.color = c.label_color or "";
        update_interval = c.update_interval or 0;
        truncation = c.truncation or 0;
        format = c.format or "";
      };
    in
    mapAttrsToList mk enabled;

  sectionFromComponents = key: components:
    let
      items = componentsToItems components;
      side = name: componentSide.${name} or "";
    in
    filter (it: side it.name == key) items;

/* Flatten every section into a list of { section, name, config } records. */
  allItemsList = cfg:
    concatLists (map (key:
        map (name: {
          section = key;
          inherit name;
          config = mergedItem cfg key name ((cfg.items.${key} or { }).${name} or { });
        })
            (attrNames (cfg.items.${key} or { }))
      ) [ "left" "right" "center" "center_left" "center_right" ]);

  deriveEvents = cfg:
    let
      compItems = componentsToItems cfg.components;
      explicit  = allItemsList cfg;
      evFor     = it:
        let t = it.config.type or "";
        in  map (e: { item = it.name; event = e; })
              ((componentEvents.${t} or [ ]) ++ (it.config.update_mask or [ ]));
      all = concatMap evFor explicit;
    in
    {
      components = compItems;
      items = unique all;
    };

  render = cfg:
    {
      format = "omabar-config";
      version = 1;
      bar = {
        position = cfg.bar.position or "top";
        height = cfg.bar.height or 40;
        width = cfg.bar.width or 0;
        margin = cfg.bar.margin or 0;
        blur_radius = cfg.bar.blur_radius or 0;
        color = cfg.bar.color or "";
        shadow = cfg.bar.shadow or false;
        shadow_color = cfg.bar.shadow_color or "";
        topmost = cfg.bar.topmost or true;
        sticky = cfg.bar.sticky or true;
        notch_width = cfg.bar.notch_width or 0;
        notch_offset = cfg.bar.notch_offset or 0;
        y_offset = cfg.bar.y_offset or 0;
        alpha = cfg.bar.alpha or 1.0;
      };
      defaults = {
        icon = cleanIcon cfg.defaults.icon;
        label = cleanLabel cfg.defaults.label;
        background = cleanBackground cfg.defaults.background;
        padding_left = cfg.defaults.padding_left or 0;
        padding_right = cfg.defaults.padding_right or 0;
      };
      items = {
        left = sectionFromComponents "left" cfg.components
               ++ itemsSectionWithTheme cfg "left" cfg.items.left;
        right = sectionFromComponents "right" cfg.components
                ++ itemsSectionWithTheme cfg "right" cfg.items.right;
        center = sectionFromComponents "center" cfg.components
                 ++ itemsSectionWithTheme cfg "center" cfg.items.center;
        center_left = itemsSectionWithTheme cfg "center_left" cfg.items.center_left;
        center_right = itemsSectionWithTheme cfg "center_right" cfg.items.center_right;
      };
      animations = {
        enable = cfg.animations.enable or false;
        duration = cfg.animations.duration or 0.2;
        function = cfg.animations.function or "ease_in_out";
      };
      daemon = {
        hotload = cfg.daemon.hotload or false;
        log_level = cfg.daemon.log_level or "info";
        pid_file = cfg.daemon.pid_file or "/tmp/omabar.pid";
        lock_file = cfg.daemon.lock_file or "/tmp/omabar.lock";
      };
      events = deriveEvents cfg;
      custom_events = mapAttrsToList (name: e:
        { inherit name; notification = e.notification or null; })
        (cfg.events or { });
      plugins = mapAttrsToList (name: p:
        { inherit name; update_interval = p.update_interval or 0;
          env = p.env or { }; path = if p.enable or false then "${p.package}" else ""; })
        (filterAttrs (_: p: p.enable or false) cfg.plugins);
    };

  itemsSection = cfg: key: namedItems:
    map (name: itemToJSON cfg key name namedItems.${name}) (attrNames namedItems);

  /* A section = user items + theme items not overridden by the user, so the
     theme keeps working as the baseline while users add/tune freely. */
  itemsSectionWithTheme = cfg: key: namedItems:
    itemsSection cfg key namedItems
    ++ itemsSection cfg key
         (filterAttrs (name: _: !(hasAttr name namedItems)) (theme.items.${key} or { }));

  /* Encode as a derivation: binary header byte-for-byte, then JSON payload.
     The payload may reference plugin store paths, so it is passed through
     passAsFile (builtins.toFile may not reference derivations). */
  generate = { cfg ? { }, name ? "omabar.config" }:
    let payload = toJSON (render cfg);
    in
    pkgs.runCommandLocal name {
      passAsFile = [ "payload" ];
      inherit payload;
    } ''
      printf '\x4f\x4d\x41\x42\x43\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' > $out
      cat "$payloadPath" >> $out
    '';

in
{
  inherit render generate componentsToItems;
}