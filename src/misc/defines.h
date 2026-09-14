#ifndef OMABAR_DEFINES_H
#define OMABAR_DEFINES_H

/* bar item types */
#define BAR_ITEM_TYPE            "bar_item"
#define BAR_COMPONENT_SPACE      "space"
#define BAR_COMPONENT_ALIAS      "alias"
#define BAR_COMPONENT_GROUP      "group"
#define BAR_COMPONENT_GRAPH      "graph"
#define BAR_COMPONENT_SLIDER     "slider"

/* bar item properties */
#define BAR_ITEM_NAME            "name"
#define BAR_ITEM_ICON            "icon"
#define BAR_ITEM_LABEL           "label"
#define BAR_ITEM_BACKGROUND      "background"
#define BAR_ITEM_POSITION        "position"
#define BAR_ITEM_UPDATE_MASK     "update_mask"
#define BAR_ITEM_HIDDEN          "hidden"
#define BAR_ITEM_CLICK_ENABLED   "click_enabled"
#define BAR_ITEM_SCROLL_ENABLED  "scroll_enabled"
#define BAR_ITEM_SCRIPT          "script"
#define BAR_ITEM_CLICK_SCRIPT    "click_script"
#define BAR_ITEM_MACH_HELPER     "mach_helper"
#define BAR_ITEM_UPDATE_INTERVAL "update_interval"
#define BAR_ITEM_Y_OFFSET        "y_offset"
#define BAR_ITEM_PADDING_LEFT    "padding_left"
#define BAR_ITEM_PADDING_RIGHT   "padding_right"
#define BAR_ITEM_LABEL_X_OFFSET  "label_x_offset"
#define BAR_ITEM_ICON_X_OFFSET   "icon_x_offset"
#define BAR_ITEM_ASSOCIATED_SPACE   "associated_space"
#define BAR_ITEM_ASSOCIATED_DISPLAY "associated_display"
#define BAR_ITEM_ASSOCIATED_BAR     "associated_bar"

/* icon properties */
#define BAR_ITEM_ICON_FONT       "icon.font"
#define BAR_ITEM_ICON_COLOR      "icon.color"
#define BAR_ITEM_ICON_HIGHLIGHT  "icon.highlight_color"
#define BAR_ITEM_ICON_STRING     "icon.string"

/* label properties */
#define BAR_ITEM_LABEL_FONT       "label.font"
#define BAR_ITEM_LABEL_COLOR      "label.color"
#define BAR_ITEM_LABEL_HIGHLIGHT  "label.highlight_color"
#define BAR_ITEM_LABEL_STRING     "label.string"
#define BAR_ITEM_LABEL_Y_OFFSET   "label.y_offset"
#define BAR_ITEM_LABEL_X_OFFSET   "label.x_offset"

/* background properties */
#define BAR_ITEM_BG_COLOR         "background.color"
#define BAR_ITEM_BG_BORDER_COLOR  "background.border_color"
#define BAR_ITEM_BG_CORNER_RADIUS "background.corner_radius"
#define BAR_ITEM_BG_BORDER_WIDTH  "background.border_width"
#define BAR_ITEM_BG_IMAGE         "background.image"
#define BAR_ITEM_BG_CLIP          "background.clip"

/* bar position */
#define BAR_POSITION_TOP          "top"
#define BAR_POSITION_BOTTOM       "bottom"
#define BAR_POSITION_LEFT         "left"
#define BAR_POSITION_RIGHT        "right"

/* bar properties */
#define BAR_POSITION              "position"
#define BAR_HEIGHT                "height"
#define BAR_WIDTH                 "width"
#define BAR_MARGIN                "margin"
#define BAR_BLUR_RADIUS           "blur_radius"
#define BAR_SHADOW                "shadow"
#define BAR_TOPMOST               "topmost"
#define BAR_STICKY                "sticky"
#define BAR_NOTCH_WIDTH           "notch_width"
#define BAR_NOTCH_OFFSET          "notch_offset"
#define BAR_Y_OFFSET              "y_offset"

/* color hex format */
#define COLOR_HEX_PREFIX          "0x"
#define COLOR_LENGTH               10  /* "0xAARRGGBB" */

#endif