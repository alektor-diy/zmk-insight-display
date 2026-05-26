#pragma once

#include <stdbool.h>
#include <stdint.h>

enum zmk_insight_display_side {
    ZMK_INSIGHT_DISPLAY_SIDE_LEFT = 0,
    ZMK_INSIGHT_DISPLAY_SIDE_RIGHT = 1,
};

enum zmk_insight_display_layout {
    ZMK_INSIGHT_DISPLAY_LAYOUT_128X32 = 0,
    ZMK_INSIGHT_DISPLAY_LAYOUT_128X64 = 1,
};

enum zmk_insight_display_screen_mode {
    ZMK_INSIGHT_DISPLAY_SCREEN_MODE_SINGLE = 0,
    ZMK_INSIGHT_DISPLAY_SCREEN_MODE_PAIRED = 1,
};

enum zmk_insight_display_side zmk_insight_display_local_side(void);
enum zmk_insight_display_layout zmk_insight_display_layout_kind(void);
enum zmk_insight_display_screen_mode zmk_insight_display_screen_mode_kind(void);
bool zmk_insight_display_has_local_display(void);
