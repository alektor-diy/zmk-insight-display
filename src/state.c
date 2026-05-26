#include <string.h>

#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/state.h>

static struct zmk_insight_display_state current_state = {
    .version = CONFIG_ZMK_INSIGHT_DISPLAY_SYNC_VERSION,
};

const struct zmk_insight_display_state *zmk_insight_display_state_ptr(void) { return &current_state; }

bool zmk_insight_display_state_get(struct zmk_insight_display_state *out) {
    if (out == NULL) {
        return false;
    }

    *out = current_state;
    return true;
}

bool zmk_insight_display_state_set(const struct zmk_insight_display_state *next) {
    if (next == NULL) {
        return false;
    }

    if (memcmp(&current_state, next, sizeof(current_state)) == 0) {
        return false;
    }

    current_state = *next;
    raise_zmk_insight_display_state_changed(
        (struct zmk_insight_display_state_changed){.state = current_state});
    return true;
}

void zmk_insight_display_state_reset(void) {
    struct zmk_insight_display_state reset = {
        .version = CONFIG_ZMK_INSIGHT_DISPLAY_SYNC_VERSION,
    };

    zmk_insight_display_state_set(&reset);
}
