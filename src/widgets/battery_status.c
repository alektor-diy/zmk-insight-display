#include <stdio.h>

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>

#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>
#include <zmk_insight_display/widgets/battery_status.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void set_battery_status(struct zmk_insight_display_widget_battery_status *widget,
                               struct zmk_insight_display_state state) {
    const bool runtime_ready = zmk_insight_display_runtime_ready();
    const bool is_left = widget->side_label == 'L';
    const bool valid =
        (state.flags & (is_left ? ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID
                                : ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID)) != 0U;
    uint8_t battery = is_left ? state.left_battery : state.right_battery;

    if (!runtime_ready) {
        lv_obj_add_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);

    if (!valid) {
        snprintf(widget->text, sizeof(widget->text), "%c:--%%", widget->side_label);
        lv_label_set_text(widget->obj, widget->text);
        return;
    }

    if (battery > 100U) {
        battery = 100U;
    }

    snprintf(widget->text, sizeof(widget->text), "%c:%u%%", widget->side_label, battery);
    lv_label_set_text(widget->obj, widget->text);
}

static void battery_status_update_cb(struct zmk_insight_display_state state) {
    struct zmk_insight_display_widget_battery_status *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct zmk_insight_display_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_insight_display_state_changed *ev = as_zmk_insight_display_state_changed(eh);

    if (ev != NULL) {
        return ev->state;
    }

    return *zmk_insight_display_state_ptr();
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_insight_battery_status, struct zmk_insight_display_state,
                            battery_status_update_cb, battery_status_get_state)
ZMK_SUBSCRIPTION(widget_insight_battery_status, zmk_insight_display_state_changed);

int zmk_insight_display_widget_battery_status_init(
    struct zmk_insight_display_widget_battery_status *widget, lv_obj_t *parent, lv_coord_t x,
    lv_coord_t y, lv_coord_t width, char side_label) {
    if (widget == NULL || parent == NULL) {
        return -1;
    }

    widget->side_label = side_label;
    widget->obj = lv_label_create(parent);
    lv_obj_set_pos(widget->obj, x, y);
    lv_obj_set_width(widget->obj, width);
    lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(widget->obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->obj, 0, 0);

    snprintf(widget->text, sizeof(widget->text), "%c:--%%", side_label);
    lv_label_set_text(widget->obj, widget->text);
    lv_obj_add_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);

    sys_slist_append(&widgets, &widget->node);
    widget_insight_battery_status_init();
    return 0;
}

void zmk_insight_display_widget_battery_status_update(
    struct zmk_insight_display_widget_battery_status *widget, uint8_t battery, bool valid) {
    ARG_UNUSED(widget);
    ARG_UNUSED(battery);
    ARG_UNUSED(valid);
}
