#pragma once

#include <zmk/event_manager.h>
#include <zmk_insight_display/state.h>

struct zmk_insight_display_state_changed {
    struct zmk_insight_display_state state;
};

ZMK_EVENT_DECLARE(zmk_insight_display_state_changed);
