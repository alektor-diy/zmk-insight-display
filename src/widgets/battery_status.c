#include <stdio.h>

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>

#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>
#include <zmk_insight_display/widgets/battery_status.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void draw_battery(lv_obj_t *canvas, uint8_t level, bool valid) {
    lv_draw_rect_dsc_t rect_fill_dsc;

    lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);
    lv_draw_rect_dsc_init(&rect_fill_dsc);
    rect_fill_dsc.bg_color = lv_color_black();
    rect_fill_dsc.bg_opa = LV_OPA_COVER;
    rect_fill_dsc.border_width = 0;

    lv_canvas_set_px(canvas, 0, 0, lv_color_black());
    lv_canvas_set_px(canvas, 4, 0, lv_color_black());

    if (!valid) {
        return;
    }

    if (level > 100U) {
        level = 100U;
    }

    if (level <= 10U) {
        lv_canvas_draw_rect(canvas, 1, 2, 3, 5, &rect_fill_dsc);
    } else if (level <= 30U) {
        lv_canvas_draw_rect(canvas, 1, 2, 3, 4, &rect_fill_dsc);
    } else if (level <= 50U) {
        lv_canvas_draw_rect(canvas, 1, 2, 3, 3, &rect_fill_dsc);
    } else if (level <= 70U) {
        lv_canvas_draw_rect(canvas, 1, 2, 3, 2, &rect_fill_dsc);
    } else if (level <= 90U) {
        lv_canvas_draw_rect(canvas, 1, 2, 3, 1, &rect_fill_dsc);
    }
}

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
        draw_battery(widget->canvas, 0U, false);
        snprintf(widget->text, sizeof(widget->text), "%c:--%%", widget->side_label);
        lv_label_set_text(widget->label, widget->text);
        return;
    }

    if (battery > 100U) {
        battery = 100U;
    }

    draw_battery(widget->canvas, battery, true);
    snprintf(widget->text, sizeof(widget->text), "%c:%u%%", widget->side_label, battery);
    lv_label_set_text(widget->label, widget->text);
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
    widget->obj = lv_obj_create(parent);
    lv_obj_set_pos(widget->obj, x, y);
    lv_obj_set_size(widget->obj, width, 12);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->obj, 0, 0);
    lv_obj_set_style_pad_all(widget->obj, 0, 0);
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);

    widget->canvas = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(widget->canvas, widget->canvas_buffer, 5, 8, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_pos(widget->canvas, 14, 2);

    widget->label = lv_label_create(widget->obj);
    lv_obj_set_pos(widget->label, 24, 0);
    lv_obj_set_width(widget->label, width - 24);
    lv_obj_set_style_text_align(widget->label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_color(widget->label, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(widget->label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->label, 0, 0);

    snprintf(widget->text, sizeof(widget->text), "%c:--%%", side_label);
    lv_label_set_text(widget->label, widget->text);
    draw_battery(widget->canvas, 0U, false);
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
