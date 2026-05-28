#include <stdio.h>

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>

#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>
#include <zmk_insight_display/widgets/layer_status.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void set_layer_status(struct zmk_insight_display_widget_layer_status *widget,
                             struct zmk_insight_display_state state) {
    const bool runtime_ready = zmk_insight_display_runtime_ready();
    const bool layer_valid = (state.flags & ZMK_INSIGHT_DISPLAY_FLAG_LAYER_VALID) != 0U;

    if (!runtime_ready) {
        lv_obj_add_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);

    if (!layer_valid) {
        lv_label_set_text(widget->obj, "-");
        return;
    }

    snprintf(widget->text, sizeof(widget->text), "%u", state.layer);
    lv_label_set_text(widget->obj, widget->text);
}

static void layer_status_update_cb(struct zmk_insight_display_state state) {
    struct zmk_insight_display_widget_layer_status *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct zmk_insight_display_state layer_status_get_state(const zmk_event_t *eh) {
    const struct zmk_insight_display_state_changed *ev = as_zmk_insight_display_state_changed(eh);

    if (ev != NULL) {
        return ev->state;
    }

    return *zmk_insight_display_state_ptr();
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_insight_layer_status, struct zmk_insight_display_state,
                            layer_status_update_cb, layer_status_get_state)
ZMK_SUBSCRIPTION(widget_insight_layer_status, zmk_insight_display_state_changed);

int zmk_insight_display_widget_layer_status_init(
    struct zmk_insight_display_widget_layer_status *widget, lv_obj_t *parent) {
    if (widget == NULL || parent == NULL) {
        return -1;
    }

    widget->obj = lv_label_create(parent);
    lv_obj_set_width(widget->obj, 24);
    lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(widget->obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->obj, 0, 0);
    lv_label_set_text(widget->obj, "-");
    lv_obj_add_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);

    sys_slist_append(&widgets, &widget->node);
    widget_insight_layer_status_init();
    return 0;
}

lv_obj_t *zmk_insight_display_widget_layer_status_obj(
    struct zmk_insight_display_widget_layer_status *widget) {
    return widget->obj;
}
