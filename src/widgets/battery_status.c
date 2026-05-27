#include <stdio.h>

#include <widgets/lv_label.h>

#include <zmk_insight_display/widgets/battery_status.h>

int zmk_insight_display_widget_battery_status_init(
    struct zmk_insight_display_widget_battery_status *widget, lv_obj_t *parent, lv_coord_t x,
    lv_coord_t y, lv_coord_t width, char side_label) {
    if (widget == NULL || parent == NULL) {
        return -1;
    }

    widget->label = lv_label_create(parent);
    lv_obj_set_pos(widget->label, x, y);
    lv_obj_set_width(widget->label, width);
    lv_obj_set_style_text_align(widget->label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(widget->label, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(widget->label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->label, 0, 0);

    snprintf(widget->text, sizeof(widget->text), "%c:--%%", side_label);
    lv_label_set_text(widget->label, widget->text);
    return 0;
}

void zmk_insight_display_widget_battery_status_update(
    struct zmk_insight_display_widget_battery_status *widget, uint8_t battery, bool valid) {
    if (widget == NULL || widget->label == NULL) {
        return;
    }

    if (!valid) {
        widget->text[2] = '-';
        widget->text[3] = '-';
        widget->text[4] = '%';
        widget->text[5] = '\0';
        lv_label_set_text(widget->label, widget->text);
        return;
    }

    if (battery > 100U) {
        battery = 100U;
    }

    snprintf(widget->text + 2, sizeof(widget->text) - 2, "%u%%", battery);
    lv_label_set_text(widget->label, widget->text);
}
