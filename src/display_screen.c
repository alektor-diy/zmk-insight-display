#include <stdio.h>
#include <string.h>

#include <lvgl.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk_insight_display/config.h>
#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/state.h>

struct zmk_insight_display_widget {
    sys_snode_t node;
    lv_obj_t *root;
    lv_obj_t *top_row;
    lv_obj_t *battery_row;
};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static const char *output_token(const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_OUTPUT_VALID) == 0U) {
        return "--";
    }

    return state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB ? "U" : "B";
}

static const char *ble_token(const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_BLE_VALID) == 0U) {
        return "-";
    }

    switch (state->ble_state) {
    case ZMK_INSIGHT_DISPLAY_BLE_STATE_CONNECTED:
        return "C";
    case ZMK_INSIGHT_DISPLAY_BLE_STATE_ADVERTISING:
        return "A";
    default:
        return "-";
    }
}

static void format_top_row(char *buf, size_t len, const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LAYER_VALID) == 0U) {
        snprintf(buf, len, "%s %s --", output_token(state), ble_token(state));
        return;
    }

    if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_BLE &&
        (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID) != 0U) {
        snprintf(buf, len, "%s %s BT%u L%u", output_token(state), ble_token(state),
                 state->profile_index + 1, state->layer);
    } else {
        snprintf(buf, len, "%s %s L%u", output_token(state), ble_token(state), state->layer);
    }
}

static void battery_text(char *buf, size_t len, uint8_t battery, bool valid) {
    if (!valid) {
        snprintf(buf, len, "--%%");
        return;
    }

    snprintf(buf, len, "%u%%", battery);
}

static void format_battery_row(char *buf, size_t len, const struct zmk_insight_display_state *state) {
    const bool left_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID) != 0U;
    const bool right_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID) != 0U;

    if (zmk_insight_display_screen_mode_kind() == ZMK_INSIGHT_DISPLAY_SCREEN_MODE_PAIRED) {
        const bool local_is_left = zmk_insight_display_local_side() == ZMK_INSIGHT_DISPLAY_SIDE_LEFT;
        battery_text(buf, len, local_is_left ? state->left_battery : state->right_battery,
                     local_is_left ? left_valid : right_valid);
        return;
    }

    char left[8];
    char right[8];
    battery_text(left, sizeof(left), state->left_battery, left_valid);
    battery_text(right, sizeof(right), state->right_battery, right_valid);
    snprintf(buf, len, "L:%s R:%s", left, right);
}

static void render_widget(struct zmk_insight_display_widget *widget,
                          const struct zmk_insight_display_state *state) {
    char top[24];
    char battery[24];

    format_top_row(top, sizeof(top), state);
    format_battery_row(battery, sizeof(battery), state);

    lv_label_set_text(widget->top_row, top);
    lv_label_set_text(widget->battery_row, battery);
}

static void update_cb(struct zmk_insight_display_state state) {
    struct zmk_insight_display_widget *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { render_widget(widget, &state); }
}

static struct zmk_insight_display_state get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return *zmk_insight_display_state_ptr();
}

ZMK_DISPLAY_WIDGET_LISTENER(zmk_insight_display_widget_listener, struct zmk_insight_display_state,
                            update_cb, get_state)
ZMK_SUBSCRIPTION(zmk_insight_display_widget_listener, zmk_insight_display_state_changed);

static void init_widget(struct zmk_insight_display_widget *widget, lv_obj_t *parent) {
    widget->root = parent;
    widget->top_row = lv_label_create(parent, NULL);
    widget->battery_row = lv_label_create(parent, NULL);

    lv_obj_align(widget->top_row, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

    if (zmk_insight_display_layout_kind() == ZMK_INSIGHT_DISPLAY_LAYOUT_128X64) {
        lv_obj_align(widget->battery_row, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 24);
    } else {
        lv_obj_align(widget->battery_row, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 16);
    }

    sys_slist_append(&widgets, &widget->node);
    zmk_insight_display_widget_listener_init();
}

lv_obj_t *zmk_display_status_screen() {
    static struct zmk_insight_display_widget widget;

    lv_obj_t *screen = lv_obj_create(NULL, NULL);
    init_widget(&widget, screen);
    return screen;
}
