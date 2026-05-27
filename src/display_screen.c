#include <stdio.h>

#include <lvgl.h>
#include <widgets/lv_label.h>

#include <zephyr/kernel.h>

#include <zmk/event_manager.h>
#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>

struct insight_display_widgets {
    lv_obj_t *screen;
    lv_obj_t *line1;
    lv_obj_t *line2;
    char line1_text[24];
    char line2_text[24];
};

static struct insight_display_widgets widgets;
static struct k_work_delayable render_work;

static const char *transport_text(const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_OUTPUT_VALID) == 0U) {
        return "--";
    }

    return state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB ? "USB" : "BLE";
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

    if (!zmk_insight_display_runtime_ready()) {
        snprintf(widgets.line1_text, sizeof(widgets.line1_text), "INITIALIZING");
        snprintf(widgets.line2_text, sizeof(widgets.line2_text), "PLEASE WAIT");
    } else {
        snprintf(widgets.line1_text, sizeof(widgets.line1_text), "%s %s L%u", transport_text(state),
                 ble_text(state), state->layer);
        snprintf(widgets.line2_text, sizeof(widgets.line2_text), "L:%s%u R:%s%u",
                 left_valid ? "" : "-", left_valid ? state->left_battery : 0,
                 right_valid ? "" : "-", right_valid ? state->right_battery : 0);
    }

    lv_label_set_text(widgets.line1, widgets.line1_text);
    lv_label_set_text(widgets.line2, widgets.line2_text);
    lv_obj_invalidate(widgets.screen);
}

static void render_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    if (widgets.screen != NULL) {
        refresh_widgets(zmk_insight_display_state_ptr());
    }
}

static int display_listener(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    (void)k_work_reschedule(&render_work, K_NO_WAIT);
    return 0;
}

ZMK_LISTENER(zmk_insight_display_display, display_listener);
ZMK_SUBSCRIPTION(zmk_insight_display_display, zmk_insight_display_state_changed);

lv_obj_t *zmk_display_status_screen(void) {
    if (widgets.screen != NULL) {
        (void)k_work_reschedule(&render_work, K_NO_WAIT);
        return widgets.screen;
    }

    k_work_init_delayable(&render_work, render_work_handler);

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

    widgets.line2 = lv_label_create(widgets.screen);
    lv_obj_set_pos(widgets.line2, 0, 18);
    lv_obj_set_width(widgets.line2, 128);
    lv_obj_set_style_text_align(widgets.line2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(widgets.line2, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(widgets.line2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widgets.line2, 0, 0);

    refresh_widgets(zmk_insight_display_state_ptr());
    return widgets.screen;
}
