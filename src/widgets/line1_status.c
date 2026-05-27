#include <stdio.h>

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>

#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>
#include <zmk_insight_display/widgets/line1_status.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

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

static void set_line1_status(struct zmk_insight_display_widget_line1_status *widget,
                             struct zmk_insight_display_state state) {
    const bool runtime_ready = zmk_insight_display_runtime_ready();
    const bool profile_valid = (state.flags & ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID) != 0U;
    const bool layer_valid = (state.flags & ZMK_INSIGHT_DISPLAY_FLAG_LAYER_VALID) != 0U;

    if (!runtime_ready) {
        snprintf(widget->text, sizeof(widget->text), "%s", boot_text());
        lv_obj_set_y(widget->obj, 11);
        lv_label_set_text(widget->obj, widget->text);
        return;
    }

    lv_obj_set_y(widget->obj, 4);
    if (state.output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB) {
        if (layer_valid) {
            snprintf(widget->text, sizeof(widget->text), "USB L%u", state.layer);
        } else {
            snprintf(widget->text, sizeof(widget->text), "USB L-");
        }
    } else if (profile_valid) {
        if (layer_valid) {
            snprintf(widget->text, sizeof(widget->text), "BT%u %s L%u",
                     state.profile_index + 1U, ble_text(&state), state.layer);
        } else {
            snprintf(widget->text, sizeof(widget->text), "BT%u %s L-",
                     state.profile_index + 1U, ble_text(&state));
        }
    } else {
        if (layer_valid) {
            snprintf(widget->text, sizeof(widget->text), "BT? %s L%u", ble_text(&state),
                     state.layer);
        } else {
            snprintf(widget->text, sizeof(widget->text), "BT? %s L-", ble_text(&state));
        }
    }

    lv_label_set_text(widget->obj, widget->text);
}

static void line1_status_update_cb(struct zmk_insight_display_state state) {
    struct zmk_insight_display_widget_line1_status *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_line1_status(widget, state); }
}

static struct zmk_insight_display_state line1_status_get_state(const zmk_event_t *eh) {
    const struct zmk_insight_display_state_changed *ev = as_zmk_insight_display_state_changed(eh);

    if (ev != NULL) {
        return ev->state;
    }

    return *zmk_insight_display_state_ptr();
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_insight_line1_status, struct zmk_insight_display_state,
                            line1_status_update_cb, line1_status_get_state)
ZMK_SUBSCRIPTION(widget_insight_line1_status, zmk_insight_display_state_changed);

int zmk_insight_display_widget_line1_status_init(
    struct zmk_insight_display_widget_line1_status *widget, lv_obj_t *parent, lv_coord_t x,
    lv_coord_t y, lv_coord_t width) {
    if (widget == NULL || parent == NULL) {
        return -1;
    }

    widget->obj = lv_label_create(parent);
    lv_obj_set_pos(widget->obj, x, y);
    lv_obj_set_width(widget->obj, width);
    lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(widget->obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->obj, 0, 0);
    lv_label_set_text(widget->obj, boot_text());

    sys_slist_append(&widgets, &widget->node);
    widget_insight_line1_status_init();
    return 0;
}

lv_obj_t *zmk_insight_display_widget_line1_status_obj(
    struct zmk_insight_display_widget_line1_status *widget) {
    return widget->obj;
}
