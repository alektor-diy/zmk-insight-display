#include <stdio.h>
#include <string.h>

#include <lvgl.h>
#include <widgets/lv_label.h>

#include <zmk/event_manager.h>
#include <zmk_insight_display/config.h>
#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>

struct insight_battery_widget {
    lv_obj_t *row;
    lv_obj_t *label;
    lv_obj_t *bar_bg;
    lv_obj_t *bar_fill;
    lv_obj_t *value;
    char label_text[4];
    char value_text[8];
};

struct insight_display_widgets {
    lv_obj_t *screen;
    lv_obj_t *boot_overlay;
    lv_obj_t *boot_title;
    lv_obj_t *boot_subtitle;
    lv_obj_t *transport_card;
    lv_obj_t *transport_value;
    lv_obj_t *ble_card;
    lv_obj_t *ble_value;
    lv_obj_t *profile_value;
    lv_obj_t *layer_card;
    lv_obj_t *layer_value;
    lv_obj_t *banner_card;
    lv_obj_t *banner_value;
    struct insight_battery_widget left_battery;
    struct insight_battery_widget right_battery;
    char transport_text[8];
    char ble_text[8];
    char profile_text[8];
    char layer_text[8];
    char banner_text[24];
};

static struct insight_display_widgets widgets;

static void apply_card_style(lv_obj_t *obj, lv_coord_t radius) {
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_white(), 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
}

static lv_obj_t *create_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t width,
                             lv_coord_t height, lv_coord_t radius) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, width, height);
    lv_obj_set_pos(obj, x, y);
    apply_card_style(obj, radius);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static lv_obj_t *create_center_label(lv_obj_t *parent, lv_coord_t y, const lv_font_t *font) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, lv_obj_get_width(parent));
    lv_obj_set_y(label, y);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(label, 0, 0);
    lv_obj_set_style_pad_all(label, 0, 0);
    if (font != NULL) {
        lv_obj_set_style_text_font(label, font, 0);
    }
    return label;
}

static void place_battery_widget(struct insight_battery_widget *widget, lv_coord_t x, lv_coord_t y,
                                 lv_coord_t width) {
    lv_coord_t bar_width = width - 28;

    if (bar_width < 18) {
        bar_width = 18;
    }

    lv_obj_set_pos(widget->row, x, y);
    lv_obj_set_size(widget->row, width, 12);
    lv_obj_set_pos(widget->bar_bg, 13, 3);
    lv_obj_set_size(widget->bar_bg, bar_width, 6);
    lv_obj_set_pos(widget->value, width - 13, 1);
}

static void init_battery_widget(struct insight_battery_widget *widget, lv_obj_t *parent,
                                lv_coord_t x, lv_coord_t y, const char *label_text) {
    widget->row = create_card(parent, x, y, 62, 12, 3);
    widget->label = lv_label_create(widget->row);
    widget->bar_bg = lv_obj_create(widget->row);
    widget->bar_fill = lv_obj_create(widget->bar_bg);
    widget->value = lv_label_create(widget->row);

    lv_label_set_text(widget->label, label_text);
    lv_obj_set_pos(widget->label, 3, 1);
    lv_obj_set_style_text_color(widget->label, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(widget->label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->label, 0, 0);

    lv_obj_set_pos(widget->bar_bg, 13, 3);
    lv_obj_set_size(widget->bar_bg, 34, 6);
    apply_card_style(widget->bar_bg, 2);
    lv_obj_clear_flag(widget->bar_bg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_pos(widget->bar_fill, 0, 0);
    lv_obj_set_size(widget->bar_fill, 0, 6);
    lv_obj_set_style_radius(widget->bar_fill, 1, 0);
    lv_obj_set_style_bg_opa(widget->bar_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(widget->bar_fill, lv_color_white(), 0);
    lv_obj_set_style_border_width(widget->bar_fill, 0, 0);
    lv_obj_clear_flag(widget->bar_fill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_pos(widget->value, 49, 1);
    lv_obj_set_style_text_color(widget->value, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(widget->value, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->value, 0, 0);

    snprintf(widget->label_text, sizeof(widget->label_text), "%s", label_text);
    snprintf(widget->value_text, sizeof(widget->value_text), "--");
    lv_label_set_text(widget->value, widget->value_text);
    place_battery_widget(widget, x, y, 62);
}

static void update_battery_widget(struct insight_battery_widget *widget, uint8_t battery,
                                  bool valid) {
    lv_coord_t fill_width = 0;
    lv_coord_t bar_width = lv_obj_get_width(widget->bar_bg);

    if (valid) {
        if (battery > 100U) {
            battery = 100U;
        }
        fill_width = (lv_coord_t)((bar_width * battery) / 100U);
        snprintf(widget->value_text, sizeof(widget->value_text), "%u", battery);
    } else {
        snprintf(widget->value_text, sizeof(widget->value_text), "--");
    }

    lv_label_set_text(widget->value, widget->value_text);
    lv_obj_set_width(widget->bar_fill, fill_width);
}

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
    const bool local_battery_valid = zmk_insight_display_local_battery_valid();

    if (!zmk_insight_display_runtime_ready()) {
        lv_obj_clear_flag(widgets.boot_overlay, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_add_flag(widgets.boot_overlay, LV_OBJ_FLAG_HIDDEN);

    snprintf(widgets.transport_text, sizeof(widgets.transport_text), "%s", transport_text(state));
    snprintf(widgets.ble_text, sizeof(widgets.ble_text), "%s", ble_text(state));
    lv_label_set_text(widgets.transport_value, widgets.transport_text);
    lv_label_set_text(widgets.ble_value, widgets.ble_text);

    if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_BLE &&
        (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID) != 0U) {
        snprintf(widgets.profile_text, sizeof(widgets.profile_text), "P%u",
                 state->profile_index + 1U);
    } else {
        snprintf(widgets.profile_text, sizeof(widgets.profile_text), "--");
    }
    lv_label_set_text(widgets.profile_value, widgets.profile_text);

    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LAYER_VALID) != 0U) {
        snprintf(widgets.layer_text, sizeof(widgets.layer_text), "L%u", state->layer);
    } else {
        snprintf(widgets.layer_text, sizeof(widgets.layer_text), "--");
    }
    lv_label_set_text(widgets.layer_value, widgets.layer_text);

    update_battery_widget(&widgets.left_battery, state->left_battery, left_valid);
    update_battery_widget(&widgets.right_battery, state->right_battery, right_valid);

    if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB && !local_battery_valid) {
        lv_obj_clear_flag(widgets.banner_card, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widgets.left_battery.row, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widgets.right_battery.row, LV_OBJ_FLAG_HIDDEN);
        snprintf(widgets.banner_text, sizeof(widgets.banner_text), "USB POWER");
    } else if (zmk_insight_display_screen_mode_kind() == ZMK_INSIGHT_DISPLAY_SCREEN_MODE_PAIRED) {
        const bool local_is_left =
            zmk_insight_display_local_side() == ZMK_INSIGHT_DISPLAY_SIDE_LEFT;

        lv_obj_add_flag(widgets.banner_card, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(widgets.left_battery.row, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widgets.right_battery.row, LV_OBJ_FLAG_HIDDEN);
        place_battery_widget(&widgets.left_battery, 0, 15, 128);
        snprintf(widgets.left_battery.label_text, sizeof(widgets.left_battery.label_text), "%s",
                 local_is_left ? "L" : "R");
        lv_label_set_text(widgets.left_battery.label, widgets.left_battery.label_text);
        update_battery_widget(&widgets.left_battery, local_is_left ? state->left_battery : state->right_battery,
                              local_is_left ? left_valid : right_valid);
        return;
    } else {
        lv_obj_add_flag(widgets.banner_card, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(widgets.left_battery.row, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(widgets.right_battery.row, LV_OBJ_FLAG_HIDDEN);
        place_battery_widget(&widgets.left_battery, 0, 15, 62);
        place_battery_widget(&widgets.right_battery, 66, 15, 62);
        lv_label_set_text(widgets.left_battery.label, "L");
        lv_label_set_text(widgets.right_battery.label, "R");
    }

    lv_label_set_text(widgets.banner_value, widgets.banner_text);
}

static int display_listener(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    if (widgets.screen != NULL) {
        refresh_widgets(zmk_insight_display_state_ptr());
    }
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
    lv_obj_set_style_bg_color(widgets.screen, lv_color_black(), 0);
    lv_obj_set_style_border_width(widgets.screen, 0, 0);
    lv_obj_set_style_pad_all(widgets.screen, 0, 0);
    lv_obj_clear_flag(widgets.screen, LV_OBJ_FLAG_SCROLLABLE);

    widgets.transport_card = create_card(widgets.screen, 0, 0, 34, 13, 4);
    widgets.transport_value = create_center_label(widgets.transport_card, 1, NULL);

    widgets.ble_card = create_card(widgets.screen, 38, 0, 34, 13, 4);
    widgets.ble_value = create_center_label(widgets.ble_card, 1, NULL);

    widgets.profile_value = lv_label_create(widgets.screen);
    lv_obj_set_pos(widgets.profile_value, 78, 2);
    lv_obj_set_style_text_color(widgets.profile_value, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(widgets.profile_value, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widgets.profile_value, 0, 0);

    widgets.layer_card = create_card(widgets.screen, 95, 0, 33, 13, 4);
    widgets.layer_value = create_center_label(widgets.layer_card, 1, NULL);

    init_battery_widget(&widgets.left_battery, widgets.screen, 0, 15, "L");
    init_battery_widget(&widgets.right_battery, widgets.screen, 66, 15, "R");

    widgets.banner_card = create_card(widgets.screen, 0, 15, 128, 12, 3);
    widgets.banner_value = create_center_label(widgets.banner_card, 1, NULL);
    lv_obj_add_flag(widgets.banner_card, LV_OBJ_FLAG_HIDDEN);

    widgets.boot_overlay = lv_obj_create(widgets.screen);
    lv_obj_set_size(widgets.boot_overlay, 128, 32);
    lv_obj_set_pos(widgets.boot_overlay, 0, 0);
    lv_obj_set_style_bg_opa(widgets.boot_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(widgets.boot_overlay, lv_color_black(), 0);
    lv_obj_set_style_border_width(widgets.boot_overlay, 0, 0);
    lv_obj_set_style_pad_all(widgets.boot_overlay, 0, 0);
    lv_obj_clear_flag(widgets.boot_overlay, LV_OBJ_FLAG_SCROLLABLE);

    widgets.boot_title = create_center_label(widgets.boot_overlay, 6, NULL);
    widgets.boot_subtitle = create_center_label(widgets.boot_overlay, 18, NULL);
    lv_label_set_text(widgets.boot_title, "INITIALIZING");
    lv_label_set_text(widgets.boot_subtitle, "PLEASE WAIT");

    refresh_widgets(zmk_insight_display_state_ptr());
    return widgets.screen;
}
