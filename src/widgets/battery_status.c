#include <stdio.h>

#include <lvgl.h>

#include <zmk_insight_display/widgets/battery_status.h>

static const char *battery_symbol(uint8_t battery, bool valid) {
    if (!valid) {
        return LV_SYMBOL_BATTERY_EMPTY;
    }

    if (battery >= 90U) {
        return LV_SYMBOL_BATTERY_FULL;
    }

    if (battery >= 65U) {
        return LV_SYMBOL_BATTERY_3;
    }

    if (battery >= 40U) {
        return LV_SYMBOL_BATTERY_2;
    }

    if (battery >= 15U) {
        return LV_SYMBOL_BATTERY_1;
    }

    return LV_SYMBOL_BATTERY_EMPTY;
}

int zmk_insight_display_widget_battery_status_init(
    struct zmk_insight_display_widget_battery_status *widget, lv_obj_t *parent, lv_coord_t x,
    lv_coord_t y, lv_coord_t width, char side_label) {
    if (widget == NULL || parent == NULL) {
        return -1;
    }

    widget->side_label = side_label;
    widget->label = lv_label_create(parent);
    lv_obj_set_pos(widget->label, x, y);
    lv_obj_set_width(widget->label, width);
    lv_obj_set_style_text_align(widget->label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(widget->label, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(widget->label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->label, 0, 0);

    snprintf(widget->text, sizeof(widget->text), "%c %s --%%", side_label,
             LV_SYMBOL_BATTERY_EMPTY);
    lv_label_set_text(widget->label, widget->text);
    return 0;
}

void zmk_insight_display_widget_battery_status_update(
    struct zmk_insight_display_widget_battery_status *widget, uint8_t battery, bool valid) {
    if (widget == NULL || widget->label == NULL) {
        return;
    }

    if (battery > 100U) {
        battery = 100U;
    }

    if (!valid) {
        snprintf(widget->text, sizeof(widget->text), "%c %s --%%", widget->side_label,
                 battery_symbol(0U, false));
        lv_label_set_text(widget->label, widget->text);
        return;
    }

    snprintf(widget->text, sizeof(widget->text), "%c %s %u%%", widget->side_label,
             battery_symbol(battery, true), battery);
    lv_label_set_text(widget->label, widget->text);
}
