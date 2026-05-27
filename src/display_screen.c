#include <stdio.h>
#include <string.h>

#include <lvgl.h>
#include <widgets/lv_label.h>

#include <zmk/event_manager.h>
#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>
#include <zmk_insight_display/widgets/battery_status.h>

struct insight_display_widgets {
    lv_obj_t *screen;
    lv_obj_t *line1;
    struct zmk_insight_display_widget_battery_status left_battery;
    struct zmk_insight_display_widget_battery_status right_battery;
    lv_timer_t *refresh_timer;
    char line1_text[24];
};

static struct insight_display_widgets widgets;
static volatile bool refresh_pending = true;
static bool last_runtime_ready;
static struct zmk_insight_display_state last_rendered_state;

static const char *boot_text(void) {
    if (sizeof(CONFIG_ZMK_INSIGHT_DISPLAY_BOOT_TEXT) <= 1U) {
        return "INITIALIZING...";
    }

    return CONFIG_ZMK_INSIGHT_DISPLAY_BOOT_TEXT;
}

static const char *ble_text(const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_BLE_VALID) == 0U) {
        return "---";
    }

    switch (state->ble_state) {
    case ZMK_INSIGHT_DISPLAY_BLE_STATE_CONNECTED:
        return "CON";
    case ZMK_INSIGHT_DISPLAY_BLE_STATE_ADVERTISING:
        return "ADV";
    default:
        return "---";
    }
}

static void refresh_widgets(const struct zmk_insight_display_state *state) {
    const bool left_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID) != 0U;
    const bool right_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID) != 0U;
    const bool profile_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID) != 0U;
    const bool layer_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LAYER_VALID) != 0U;

    if (!zmk_insight_display_runtime_ready()) {
        snprintf(widgets.line1_text, sizeof(widgets.line1_text), "%s", boot_text());
        lv_obj_set_y(widgets.line1, 11);
        lv_obj_add_flag(widgets.left_battery.root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widgets.right_battery.root, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_set_y(widgets.line1, 4);
        lv_obj_clear_flag(widgets.left_battery.root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(widgets.right_battery.root, LV_OBJ_FLAG_HIDDEN);
        if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB) {
            snprintf(widgets.line1_text, sizeof(widgets.line1_text), "USB %s",
                     layer_valid ? "L0" : "L-");
            if (layer_valid) {
                snprintf(widgets.line1_text, sizeof(widgets.line1_text), "USB L%u", state->layer);
            }
        } else if (profile_valid) {
            snprintf(widgets.line1_text, sizeof(widgets.line1_text), "BT%u %s %s",
                     state->profile_index + 1U, ble_text(state), layer_valid ? "L0" : "L-");
            if (layer_valid) {
                snprintf(widgets.line1_text, sizeof(widgets.line1_text), "BT%u %s L%u",
                         state->profile_index + 1U, ble_text(state), state->layer);
            }
        } else {
            snprintf(widgets.line1_text, sizeof(widgets.line1_text), "BT? %s %s", ble_text(state),
                     layer_valid ? "L0" : "L-");
            if (layer_valid) {
                snprintf(widgets.line1_text, sizeof(widgets.line1_text), "BT? %s L%u",
                         ble_text(state), state->layer);
            }
        }
        zmk_insight_display_widget_battery_status_update(&widgets.left_battery, state->left_battery,
                                                         left_valid);
        zmk_insight_display_widget_battery_status_update(&widgets.right_battery,
                                                         state->right_battery, right_valid);
    }

    lv_label_set_text(widgets.line1, widgets.line1_text);
    lv_obj_invalidate(widgets.screen);
}

static void refresh_timer_cb(lv_timer_t *timer) {
    const struct zmk_insight_display_state *state;
    const bool runtime_ready = zmk_insight_display_runtime_ready();

    ARG_UNUSED(timer);

    if (widgets.screen == NULL) {
        return;
    }

    state = zmk_insight_display_state_ptr();
    if (!refresh_pending && runtime_ready == last_runtime_ready &&
        memcmp(&last_rendered_state, state, sizeof(last_rendered_state)) == 0) {
        return;
    }

    refresh_pending = false;
    last_runtime_ready = runtime_ready;
    last_rendered_state = *state;
    refresh_widgets(state);
}

static int display_listener(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    refresh_pending = true;
    return 0;
}

ZMK_LISTENER(zmk_insight_display_display, display_listener);
ZMK_SUBSCRIPTION(zmk_insight_display_display, zmk_insight_display_state_changed);

lv_obj_t *zmk_display_status_screen(void) {
    if (widgets.screen != NULL) {
        refresh_widgets(zmk_insight_display_state_ptr());
        return widgets.screen;
    }

    widgets.screen = lv_obj_create(NULL);
    lv_obj_set_size(widgets.screen, 128, 32);
    lv_obj_set_style_bg_opa(widgets.screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(widgets.screen, lv_color_white(), 0);
    lv_obj_set_style_border_width(widgets.screen, 0, 0);
    lv_obj_set_style_pad_all(widgets.screen, 0, 0);
    lv_obj_clear_flag(widgets.screen, LV_OBJ_FLAG_SCROLLABLE);

    widgets.line1 = lv_label_create(widgets.screen);
    lv_obj_set_pos(widgets.line1, 0, 4);
    lv_obj_set_width(widgets.line1, 128);
    lv_obj_set_style_text_align(widgets.line1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(widgets.line1, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(widgets.line1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widgets.line1, 0, 0);

    (void)zmk_insight_display_widget_battery_status_init(&widgets.left_battery, widgets.screen, 0,
                                                         18, 64, 'L');
    (void)zmk_insight_display_widget_battery_status_init(&widgets.right_battery, widgets.screen, 64,
                                                         18, 64, 'R');

    widgets.refresh_timer = lv_timer_create(refresh_timer_cb, 33, NULL);
    refresh_pending = true;
    last_runtime_ready = false;
    memset(&last_rendered_state, 0, sizeof(last_rendered_state));
    refresh_widgets(zmk_insight_display_state_ptr());
    return widgets.screen;
}
