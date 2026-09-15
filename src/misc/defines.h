#ifndef OMABAR_DEFINES_H
#define OMABAR_DEFINES_H

/* ═══════════════════════════════════════════════════════════════
   IPC / CLI command surface (compatible with SketchyBar's --add /
   --remove / --set / --query protocol for plugins & scripting)
   ═══════════════════════════════════════════════════════════════ */

#define DOMAIN_ADD                             "--add"
#define COMMAND_ADD_ITEM                       "item"
#define COMMAND_ADD_COMPONENT                  "component"
#define COMMAND_ADD_EVENT                      "event"

#define DOMAIN_UPDATE                          "--update"
#define DOMAIN_PUSH                            "--push"
#define DOMAIN_TRIGGER                         "--trigger"
#define DOMAIN_DEFAULT                         "--default"
#define COMMAND_DEFAULT_RESET                  "reset"
#define DOMAIN_CLONE                           "--clone"
#define DOMAIN_RENAME                          "--rename"
#define DOMAIN_REORDER                         "--reorder"
#define DOMAIN_REMOVE                          "--remove"
#define DOMAIN_MOVE                            "--move"
#define DOMAIN_SET                             "--set"
#define DOMAIN_ANIMATE                         "--animate"
#define DOMAIN_EXIT                            "--exit"
#define DOMAIN_HOTLOAD                         "--hotload"
#define DOMAIN_RELOAD                          "--reload"
#define DOMAIN_ADD_FONT                        "--load-font"

#define SUB_DOMAIN_ICON                        "icon"
#define SUB_DOMAIN_LABEL                       "label"
#define SUB_DOMAIN_BACKGROUND                  "background"
#define SUB_DOMAIN_GRAPH                       "graph"
#define SUB_DOMAIN_ALIAS                       "alias"
#define SUB_DOMAIN_POPUP                       "popup"
#define SUB_DOMAIN_SHADOW                      "shadow"
#define SUB_DOMAIN_IMAGE                       "image"
#define SUB_DOMAIN_KNOB                        "knob"
#define SUB_DOMAIN_SLIDER                      "slider"
#define SUB_DOMAIN_FONT                        "font"
#define SUB_DOMAIN_COLOR                       "color"
#define SUB_DOMAIN_BORDER_COLOR                "border_color"
#define SUB_DOMAIN_HIGHLIGHT_COLOR             "highlight_color"
#define SUB_DOMAIN_FILL_COLOR                  "fill_color"

#define DOMAIN_SUBSCRIBE                       "--subscribe"
#define DOMAIN_QUERY                           "--query"

#define COMMAND_QUERY_DEFAULT_ITEMS            "default_menu_items"
#define COMMAND_QUERY_ITEM                     "item"
#define COMMAND_QUERY_DEFAULTS                 "defaults"
#define COMMAND_QUERY_BAR                      "bar"
#define COMMAND_QUERY_EVENTS                   "events"
#define COMMAND_QUERY_DISPLAYS                 "displays"

/* ═══════════════════════════════════════════════════════════════
   Event names
   ═══════════════════════════════════════════════════════════════ */

#define EVENT_FRONT_APP_SWITCHED        "front_app_switched"
#define EVENT_SPACE_CHANGE              "space_change"
#define EVENT_DISPLAY_CHANGE            "display_change"
#define EVENT_SYSTEM_WOKE               "system_woke"
#define EVENT_SYSTEM_WILL_SLEEP         "system_will_sleep"
#define EVENT_VOLUME_CHANGE             "volume_change"
#define EVENT_WIFI_CHANGE               "wifi_change"
#define EVENT_BRIGHTNESS_CHANGE         "brightness_change"
#define EVENT_POWER_SOURCE_CHANGE       "power_source_change"
#define EVENT_MEDIA_CHANGE              "media_change"
#define EVENT_MOUSE_ENTERED             "mouse.entered"
#define EVENT_MOUSE_EXITED              "mouse.exited"
#define EVENT_MOUSE_CLICKED             "mouse.clicked"
#define EVENT_MOUSE_SCROLLED            "mouse.scrolled"
#define EVENT_MOUSE_ENTERED_GLOBAL      "mouse.entered.global"
#define EVENT_MOUSE_EXITED_GLOBAL       "mouse.exited.global"
#define EVENT_MOUSE_SCROLLED_GLOBAL     "mouse.scrolled.global"
#define EVENT_SPACE_WINDOWS_CHANGE      "space_windows_change"

/* ═══════════════════════════════════════════════════════════════
   Bar item / component types
   ═══════════════════════════════════════════════════════════════ */

#define TYPE_GRAPH                       "graph"
#define TYPE_SPACE                       "space"
#define TYPE_ALIAS                       "alias"
#define TYPE_GROUP                       "bracket"
#define TYPE_SLIDER                      "slider"
#define TYPE_ITEM                        "item"

#define BAR_ITEM_TYPE                    "bar_item"
#define BAR_COMPONENT_SPACE              "space"
#define BAR_COMPONENT_ALIAS              "alias"
#define BAR_COMPONENT_GROUP              "group"
#define BAR_COMPONENT_GRAPH              "graph"
#define BAR_COMPONENT_SLIDER             "slider"

/* ═══════════════════════════════════════════════════════════════
   Bar position types (char codes, SketchyBar compatible)
   ═══════════════════════════════════════════════════════════════ */

#define POSITION_TOP                     't'
#define POSITION_BOTTOM                  'b'
#define POSITION_LEFT                    'l'
#define POSITION_RIGHT                   'r'
#define POSITION_CENTER                  'c'
#define POSITION_POPUP                   'p'
#define POSITION_CENTER_LEFT             'q'
#define POSITION_CENTER_RIGHT            'e'

/* string forms for Nix config / IPC */
#define BAR_POSITION_TOP                 "top"
#define BAR_POSITION_BOTTOM              "bottom"
#define BAR_POSITION_LEFT                "left"
#define BAR_POSITION_RIGHT               "right"
#define BAR_POSITION_CENTER              "center"

/* ═══════════════════════════════════════════════════════════════
   Bar item properties
   ═══════════════════════════════════════════════════════════════ */

#define BAR_ITEM_NAME                    "name"
#define BAR_ITEM_ICON                    "icon"
#define BAR_ITEM_LABEL                   "label"
#define BAR_ITEM_BACKGROUND              "background"
#define BAR_ITEM_POSITION                "position"
#define BAR_ITEM_UPDATE_MASK             "update_mask"
#define BAR_ITEM_HIDDEN                  "hidden"
#define BAR_ITEM_CLICK_ENABLED           "click_enabled"
#define BAR_ITEM_SCROLL_ENABLED          "scroll_enabled"
#define BAR_ITEM_SCRIPT                  "script"
#define BAR_ITEM_CLICK_SCRIPT            "click_script"
#define BAR_ITEM_MACH_HELPER             "mach_helper"
#define BAR_ITEM_UPDATE_INTERVAL         "update_interval"
#define BAR_ITEM_Y_OFFSET                "y_offset"
#define BAR_ITEM_PADDING_LEFT            "padding_left"
#define BAR_ITEM_PADDING_RIGHT           "padding_right"
#define BAR_ITEM_LABEL_X_OFFSET          "label_x_offset"
#define BAR_ITEM_ICON_X_OFFSET           "icon_x_offset"
#define BAR_ITEM_ASSOCIATED_SPACE        "associated_space"
#define BAR_ITEM_ASSOCIATED_DISPLAY      "associated_display"
#define BAR_ITEM_ASSOCIATED_BAR          "associated_bar"

#define PROPERTY_FONT                    "font"
#define PROPERTY_COLOR                   "color"
#define PROPERTY_HIGHLIGHT               "highlight"
#define PROPERTY_HIGHLIGHT_COLOR         "highlight_color"
#define PROPERTY_PADDING_LEFT            "padding_left"
#define PROPERTY_PADDING_RIGHT           "padding_right"
#define PROPERTY_HEIGHT                  "height"
#define PROPERTY_BORDER_COLOR            "border_color"
#define PROPERTY_BORDER_WIDTH            "border_width"
#define PROPERTY_CORNER_RADIUS           "corner_radius"
#define PROPERTY_FILL_COLOR              "fill_color"
#define PROPERTY_LINE_WIDTH              "line_width"
#define PROPERTY_BLUR_RADIUS             "blur_radius"
#define PROPERTY_DRAWING                 "drawing"
#define PROPERTY_CLIP                    "clip"
#define PROPERTY_DISTANCE                "distance"
#define PROPERTY_ANGLE                   "angle"
#define PROPERTY_SCALE                   "scale"
#define PROPERTY_STRING                  "string"
#define PROPERTY_SCROLL_TEXTS            "scroll_texts"
#define PROPERTY_SCROLL_DURATION         "scroll_duration"

#define PROPERTY_COLOR_HEX               "hex"
#define PROPERTY_COLOR_ALPHA             "alpha"
#define PROPERTY_COLOR_RED               "red"
#define PROPERTY_COLOR_GREEN             "green"
#define PROPERTY_COLOR_BLUE              "blue"

#define PROPERTY_FONT_FAMILY             "family"
#define PROPERTY_FONT_STYLE              "style"
#define PROPERTY_FONT_SIZE               "size"
#define PROPERTY_FONT_FEATURES           "features"

#define PROPERTY_UPDATES                 "updates"
#define PROPERTY_POSITION                "position"
#define PROPERTY_ASSOCIATED_DISPLAY      "associated_display"
#define PROPERTY_ASSOCIATED_SPACE        "associated_space"
#define PROPERTY_UPDATE_FREQ             "update_freq"
#define PROPERTY_SCRIPT                  "script"
#define PROPERTY_CLICK_SCRIPT            "click_script"
#define PROPERTY_ICON                    "icon"
#define PROPERTY_XOFFSET                 "x_offset"
#define PROPERTY_YOFFSET                 "y_offset"
#define PROPERTY_WIDTH                   "width"
#define PROPERTY_LABEL                   "label"
#define PROPERTY_CACHE_SCRIPTS           "cache_scripts"
#define PROPERTY_LAZY                    "lazy"
#define PROPERTY_IGNORE_ASSOCIATION      "ignore_association"
#define PROPERTY_EVENT_PORT              "mach_helper"
#define PROPERTY_PERCENTAGE              "percentage"
#define PROPERTY_MAX_CHARS               "max_chars"

/* icon / label sub-properties */
#define BAR_ITEM_ICON_FONT               "icon.font"
#define BAR_ITEM_ICON_COLOR              "icon.color"
#define BAR_ITEM_ICON_HIGHLIGHT          "icon.highlight_color"
#define BAR_ITEM_ICON_STRING             "icon.string"
#define BAR_ITEM_LABEL_FONT              "label.font"
#define BAR_ITEM_LABEL_COLOR             "label.color"
#define BAR_ITEM_LABEL_HIGHLIGHT         "label.highlight_color"
#define BAR_ITEM_LABEL_STRING            "label.string"
#define BAR_ITEM_BG_COLOR                "background.color"
#define BAR_ITEM_BG_BORDER_COLOR         "background.border_color"
#define BAR_ITEM_BG_CORNER_RADIUS        "background.corner_radius"
#define BAR_ITEM_BG_BORDER_WIDTH         "background.border_width"
#define BAR_ITEM_BG_IMAGE                "background.image"
#define BAR_ITEM_BG_CLIP                 "background.clip"

/* ═══════════════════════════════════════════════════════════════
   Bar properties
   ═══════════════════════════════════════════════════════════════ */

#define BAR_POSITION                     "position"
#define BAR_HEIGHT                       "height"
#define BAR_WIDTH                        "width"
#define BAR_MARGIN                       "margin"
#define BAR_BLUR_RADIUS                  "blur_radius"
#define BAR_SHADOW                       "shadow"
#define BAR_TOPMOST                      "topmost"
#define BAR_STICKY                       "sticky"
#define BAR_NOTCH_WIDTH                  "notch_width"
#define BAR_NOTCH_OFFSET                 "notch_offset"
#define BAR_NOTCH_DISPLAY_HEIGHT         "notch_display_height"
#define BAR_Y_OFFSET                     "y_offset"
#define BAR_DISPLAY                      "display"
#define BAR_SPACE                        "space"
#define BAR_SHOW_IN_FULLSCREEN           "show_in_fullscreen"
#define BAR_FONT_SMOOTHING               "font_smoothing"
#define BAR_ALIGN                        "align"
#define BAR_HORIZONTAL                   "horizontal"

/* ═══════════════════════════════════════════════════════════════
   Boolean / argument values
   ═══════════════════════════════════════════════════════════════ */

#define ARGUMENT_COMMON_VAL_ON           "on"
#define ARGUMENT_COMMON_VAL_NOT_OFF      "!off"
#define ARGUMENT_COMMON_VAL_TRUE         "true"
#define ARGUMENT_COMMON_VAL_NOT_FALSE    "!false"
#define ARGUMENT_COMMON_VAL_ONE          "1"
#define ARGUMENT_COMMON_VAL_NOT_ZERO     "!0"
#define ARGUMENT_COMMON_VAL_YES          "yes"
#define ARGUMENT_COMMON_VAL_NOT_NO       "!no"
#define ARGUMENT_COMMON_VAL_OFF          "off"
#define ARGUMENT_COMMON_VAL_NOT_ON       "!on"
#define ARGUMENT_COMMON_VAL_FALSE        "false"
#define ARGUMENT_COMMON_VAL_NOT_TRUE     "!true"
#define ARGUMENT_COMMON_VAL_ZERO         "0"
#define ARGUMENT_COMMON_VAL_NOT_ONE      "!1"
#define ARGUMENT_COMMON_VAL_NO           "no"
#define ARGUMENT_COMMON_VAL_NOT_YES      "!yes"
#define ARGUMENT_COMMON_VAL_TOGGLE       "toggle"
#define ARGUMENT_COMMON_VAL_BEFORE       "before"
#define ARGUMENT_COMMON_VAL_AFTER        "after"

#define ARGUMENT_DISPLAY_MAIN            "main"
#define ARGUMENT_DISPLAY_ALL             "all"

#define ARGUMENT_UPDATES_WHEN_SHOWN      "when_shown"
#define ARGUMENT_DYNAMIC                 "dynamic"

#define ARGUMENT_WINDOW                  "window"

/* ═══════════════════════════════════════════════════════════════
   Omabar-specific: keys for the Nix-generated configuration
   (attribute names in the nix module's options.schema)
   ═══════════════════════════════════════════════════════════════ */

#define OMABAR_CONFIG_SECTION_BAR        "bar"
#define OMABAR_CONFIG_SECTION_ITEMS      "items"
#define OMABAR_CONFIG_SECTION_GLOBAL     "global"
#define OMABAR_CONFIG_SECTION_PLUGINS    "plugins"

#define OMABAR_CONFIG_KEY_REF_ITEMS      "items"
#define OMABAR_CONFIG_KEY_REF_GLOBAL     "global"
#define OMABAR_CONFIG_KEY_REF_SDK        "plugin_sdk"
#define OMABAR_CONFIG_KEY_EVENTS         "events"
#define OMABAR_CONFIG_KEY_ASSOCIATED_SPACE "associated_space"
#define OMABAR_CONFIG_KEY_ASSOCIATED_DISPLAY "associated_display"
#define OMABAR_CONFIG_KEY_ASSOCIATED_BAR "associated_bar"

#define OMABAR_STATUS_OK                 "ok"
#define OMABAR_STATUS_ERROR              "error"

/* ═══════════════════════════════════════════════════════════════
   Misc
   ═══════════════════════════════════════════════════════════════ */

/* color hex format: "0xAARRGGBB" */
#define COLOR_HEX_PREFIX                 "0x"
#define COLOR_LENGTH                     10

#define REGEX_DELIMITER                  '/'

#define OMABAR_NAME                      "omabar"
#define OMABAR_LOCKFILE                  "/tmp/omabar_"
#define OMABAR_LOCKFILE_SUFFIX           ".lock"

#endif