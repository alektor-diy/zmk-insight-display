#include <stdio.h>
#include <string.h>

#include <lvgl.h>
#include <widgets/lv_label.h>

#include <zmk/event_manager.h>
#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>

struct insight_battery_view {
    lv_obj_t *card;
    lv_obj_t *label;
    lv_obj_t *value;
    lv_obj_t *bar_bg;
    lv_obj_t *bar_fill;
    char value_text[8];
};

struct insight_display_widgets {
    lv_obj_t *screen;
    lv_obj_t *top_left_card;
    lv_obj_t *top_left_label;
    lv_obj_t *top_mid_card;
    lv_obj_t *top_mid_label;
    lv_obj_t *top_right_card;
    lv_obj_t *top_right_label;
    struct insight_battery_view left_battery;
    struct insight_battery_view right_battery;
    lv_timer_t *refresh_timer;
    char top_left_text[12];
    char top_mid_text[12];
    char top_right_text[12];
};

static struct insight_display_widgets widgets;
static volatile bool refresh_pending = true;
static bool last_runtime_ready;
static struct zmk_insight_display_state last_rendered_state;

static void apply_card_style(lv_obj_t *obj, lv_color_t bg, lv_color_t border, lv_coord_t radius) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, bg, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, border, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *create_label(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t width,
                              lv_text_align_t align, bool white_text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, width);
    lv_obj_set_style_text_align(label, align, 0);
    lv_obj_set_style_text_color(label, white_text ? lv_color_white() : lv_color_black(), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(label, 0, 0);
    lv_obj_set_style_pad_all(label, 0, 0);
    return label;
}

static void init_battery_view(struct insight_battery_view *view, lv_obj_t *parent, lv_coord_t x,
                              const char *label_text) {
    view->card = lv_obj_create(parent);
    lv_obj_set_pos(view->card, x, 16);
    lv_obj_set_size(view->card, 62, 14);
    apply_card_style(view->card, lv_color_white(), lv_color_black(), 3);

    view->label = create_label(view->card, 4, 2, 8, LV_TEXT_ALIGN_LEFT, false);
    lv_label_set_text(view->label, label_text);

    view->bar_bg = lv_obj_create(view->card);
    lv_obj_set_pos(view->bar_bg, 14, 4);
    lv_obj_set_size(view->bar_bg, 30, 6);
    apply_card_style(view->bar_bg, lv_color_white(), lv_color_black(), 2);

    view->bar_fill = lv_obj_create(view->bar_bg);
    lv_obj_set_pos(view->bar_fill, 0, 0);
    lv_obj_set_size(view->bar_fill, 0, 6);
    lv_obj_set_style_bg_opa(view->bar_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(view->bar_fill, lv_color_black(), 0);
    lv_obj_set_style_border_width(view->bar_fill, 0, 0);
    lv_obj_set_style_radius(view->bar_fill, 1, 0);
    lv_obj_clear_flag(view->bar_fill, LV_OBJ_FLAG_SCROLLABLE);

    view->value = create_label(view->card, 46, 2, 14, LV_TEXT_ALIGN_CENTER, false);
    lv_label_set_text(view->value, "--");
}

static const char *ble_state_text(const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_BLE_VALID) == 0U) {
        return "--";
    }

    switch (state->ble_state) {
    case ZMK_INSIGHT_DISPLAY_BLE_STATE_CONNECTED:
        return "CON";
    case ZMK_INSIGHT_DISPLAY_BLE_STATE_ADVERTISING:
        return "ADV";
    default:
        return "--";
    }
}

static void update_battery_view(struct insight_battery_view *view, uint8_t percent, bool valid) {
    lv_coord_t fill = 0;

    if (valid) {
        if (percent > 100U) {
            percent = 100U;
        }
        fill = (lv_coord_t)((30U * percent) / 100U);
        snprintf(view->value_text, sizeof(view->value_text), "%u", percent);
    } else {
        snprintf(view->value_text, sizeof(view->value_text), "--");
    }

    lv_obj_set_width(view->bar_fill, fill);
    lv_label_set_text(view->value, view->value_text);
}

static void refresh_widgets(const struct zmk_insight_display_state *state) {
    const bool left_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID) != 0U;
    const bool right_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID) != 0U;
    const bool profile_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID) != 0U;
    const bool layer_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LAYER_VALID) != 0U;

    if (!zmk_insight_display_runtime_ready()) {
        snprintf(widgets.top_left_text, sizeof(widgets.top_left_text), "--");
        snprintf(widgets.top_mid_text, sizeof(widgets.top_mid_text), "--");
        snprintf(widgets.top_right_text, sizeof(widgets.top_right_text), "--");
        update_battery_view(&widgets.left_battery, 0U, false);
        update_battery_view(&widgets.right_battery, 0U, false);
    } else {
        if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB) {
            snprintf(widgets.top_left_text, sizeof(widgets.top_left_text), "USB");
            snprintf(widgets.top_mid_text, sizeof(widgets.top_mid_text), "LIVE");
        } else if (profile_valid) {
            snprintf(widgets.top_left_text, sizeof(widgets.top_left_text), "BT%u",
                     state->profile_index + 1U);
            snprintf(widgets.top_mid_text, sizeof(widgets.top_mid_text), "%s", ble_state_text(state));
        } else {
            snprintf(widgets.top_left_text, sizeof(widgets.top_left_text), "BT?");
            snprintf(widgets.top_mid_text, sizeof(widgets.top_mid_text), "%s", ble_state_text(state));
        }

        if (layer_valid) {
            snprintf(widgets.top_right_text, sizeof(widgets.top_right_text), "L%u", state->layer);
        } else {
            snprintf(widgets.top_right_text, sizeof(widgets.top_right_text), "L-");
        }

        update_battery_view(&widgets.left_battery, state->left_battery, left_valid);
        update_battery_view(&widgets.right_battery, state->right_battery, right_valid);
    }

    lv_label_set_text(widgets.top_left_label, widgets.top_left_text);
    lv_label_set_text(widgets.top_mid_label, widgets.top_mid_text);
    lv_label_set_text(widgets.top_right_label, widgets.top_right_text);
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
    lv_obj_set_style_pad_all(widgets.screen, 1, 0);
    lv_obj_clear_flag(widgets.screen, LV_OBJ_FLAG_SCROLLABLE);

    widgets.top_left_card = lv_obj_create(widgets.screen);
    lv_obj_set_pos(widgets.top_left_card, 0, 1);
    lv_obj_set_size(widgets.top_left_card, 38, 12);
    apply_card_style(widgets.top_left_card, lv_color_black(), lv_color_black(), 4);
    widgets.top_left_label = create_label(widgets.top_left_card, 0, 1, 38, LV_TEXT_ALIGN_CENTER, true);

    widgets.top_mid_card = lv_obj_create(widgets.screen);
    lv_obj_set_pos(widgets.top_mid_card, 43, 1);
    lv_obj_set_size(widgets.top_mid_card, 42, 12);
    apply_card_style(widgets.top_mid_card, lv_color_white(), lv_color_black(), 4);
    widgets.top_mid_label = create_label(widgets.top_mid_card, 0, 1, 42, LV_TEXT_ALIGN_CENTER, false);

    widgets.top_right_card = lv_obj_create(widgets.screen);
    lv_obj_set_pos(widgets.top_right_card, 91, 1);
    lv_obj_set_size(widgets.top_right_card, 36, 12);
    apply_card_style(widgets.top_right_card, lv_color_black(), lv_color_black(), 4);
    widgets.top_right_label =
        create_label(widgets.top_right_card, 0, 1, 36, LV_TEXT_ALIGN_CENTER, true);

    init_battery_view(&widgets.left_battery, widgets.screen, 0, "L");
    init_battery_view(&widgets.right_battery, widgets.screen, 66, "R");

    widgets.refresh_timer = lv_timer_create(refresh_timer_cb, 33, NULL);
    refresh_pending = true;
    last_runtime_ready = false;
    memset(&last_rendered_state, 0, sizeof(last_rendered_state));
    refresh_widgets(zmk_insight_display_state_ptr());
    return widgets.screen;
}
