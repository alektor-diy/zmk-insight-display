#include <stdio.h>

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>

#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>
#include <zmk_insight_display/widgets/battery_status.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_object {
    lv_obj_t *symbol;
    lv_obj_t *label;
};

static struct battery_object battery_objects[2];
static lv_color_t battery_image_buffer[2][5 * 8];

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

static void set_battery_line(uint8_t index, uint8_t level, bool valid, bool runtime_ready) {
    static const char side_labels[] = {'L', 'R'};
    char text[12];

    if (index >= 2) {
        return;
    }

    if (!runtime_ready) {
        lv_obj_add_flag(battery_objects[index].symbol, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_objects[index].label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    draw_battery(battery_objects[index].symbol, level, valid);

    if (!valid) {
        snprintf(text, sizeof(text), "%c:--%%", side_labels[index]);
    } else {
        snprintf(text, sizeof(text), "%c:%u%%", side_labels[index], level > 100U ? 100U : level);
    }

    lv_label_set_text(battery_objects[index].label, text);
    lv_obj_clear_flag(battery_objects[index].symbol, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(battery_objects[index].label, LV_OBJ_FLAG_HIDDEN);
}

static void set_battery_status(struct zmk_insight_display_state state) {
    const bool runtime_ready = zmk_insight_display_runtime_ready();
    const bool left_valid = (state.flags & ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID) != 0U;
    const bool right_valid = (state.flags & ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID) != 0U;

    set_battery_line(0, state.left_battery, left_valid, runtime_ready);
    set_battery_line(1, state.right_battery, right_valid, runtime_ready);
}

static void battery_status_update_cb(struct zmk_insight_display_state state) {
    struct zmk_insight_display_widget_battery_status *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        ARG_UNUSED(widget);
        set_battery_status(state);
    }
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
    struct zmk_insight_display_widget_battery_status *widget, lv_obj_t *parent) {
    int i;

    if (widget == NULL || parent == NULL) {
        return -1;
    }

    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->obj, 0, 0);
    lv_obj_set_style_pad_all(widget->obj, 0, 0);
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);

    for (i = 0; i < 2; i++) {
        lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
        lv_obj_t *battery_label = lv_label_create(widget->obj);

        lv_canvas_set_buffer(image_canvas, battery_image_buffer[i], 5, 8, LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(image_canvas, LV_ALIGN_TOP_RIGHT, 0, i * 10);
        lv_obj_align(battery_label, LV_ALIGN_TOP_RIGHT, -7, i * 10);

        lv_obj_add_flag(image_canvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label, LV_OBJ_FLAG_HIDDEN);

        battery_objects[i] = (struct battery_object){
            .symbol = image_canvas,
            .label = battery_label,
        };
    }

    sys_slist_append(&widgets, &widget->node);
    widget_insight_battery_status_init();
    return 0;
}

lv_obj_t *zmk_insight_display_widget_battery_status_obj(
    struct zmk_insight_display_widget_battery_status *widget) {
    return widget->obj;
}
