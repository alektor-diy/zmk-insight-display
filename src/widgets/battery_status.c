#include <stdio.h>

#include <lvgl.h>

#include <zmk_insight_display/widgets/battery_status.h>

#define BATTERY_ICON_X 12
#define BATTERY_ICON_Y 1
#define BATTERY_ICON_W 16
#define BATTERY_ICON_H 8
#define BATTERY_CAP_W 2
#define BATTERY_CAP_H 4
#define BATTERY_FILL_PAD 1

static void style_transparent(lv_obj_t *obj) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static void style_text(lv_obj_t *obj, lv_text_align_t align) {
    lv_obj_set_style_text_color(obj, lv_color_black(), 0);
    lv_obj_set_style_text_align(obj, align, 0);
    style_transparent(obj);
}

static void update_fill(struct zmk_insight_display_widget_battery_status *widget, uint8_t battery,
                        bool valid) {
    const lv_coord_t inner_w = BATTERY_ICON_W - (BATTERY_FILL_PAD * 2);
    const lv_coord_t inner_h = BATTERY_ICON_H - (BATTERY_FILL_PAD * 2);
    lv_coord_t fill_w = 0;

    if (valid) {
        if (battery > 100U) {
            battery = 100U;
        }

        fill_w = (lv_coord_t)((inner_w * battery) / 100U);
        if (battery > 0U && fill_w == 0) {
            fill_w = 1;
        }
    }

    lv_obj_set_pos(widget->fill, BATTERY_ICON_X + BATTERY_FILL_PAD, BATTERY_ICON_Y + BATTERY_FILL_PAD);
    lv_obj_set_size(widget->fill, fill_w, inner_h);
}

int zmk_insight_display_widget_battery_status_init(
    struct zmk_insight_display_widget_battery_status *widget, lv_obj_t *parent, lv_coord_t x,
    lv_coord_t y, lv_coord_t width, char side_label) {
    if (widget == NULL || parent == NULL) {
        return -1;
    }

    widget->side_label = side_label;

    widget->root = lv_obj_create(parent);
    lv_obj_set_pos(widget->root, x, y);
    lv_obj_set_size(widget->root, width, 12);
    style_transparent(widget->root);
    lv_obj_clear_flag(widget->root, LV_OBJ_FLAG_SCROLLABLE);

    widget->side = lv_label_create(widget->root);
    lv_obj_set_pos(widget->side, 0, 0);
    lv_obj_set_width(widget->side, 10);
    style_text(widget->side, LV_TEXT_ALIGN_LEFT);
    snprintf(widget->text, sizeof(widget->text), "%c:", side_label);
    lv_label_set_text(widget->side, widget->text);

    widget->outline = lv_obj_create(widget->root);
    lv_obj_set_pos(widget->outline, BATTERY_ICON_X, BATTERY_ICON_Y);
    lv_obj_set_size(widget->outline, BATTERY_ICON_W, BATTERY_ICON_H);
    lv_obj_set_style_bg_opa(widget->outline, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->outline, 1, 0);
    lv_obj_set_style_border_color(widget->outline, lv_color_black(), 0);
    lv_obj_set_style_radius(widget->outline, 0, 0);
    lv_obj_set_style_pad_all(widget->outline, 0, 0);
    lv_obj_clear_flag(widget->outline, LV_OBJ_FLAG_SCROLLABLE);

    widget->fill = lv_obj_create(widget->root);
    lv_obj_set_style_bg_opa(widget->fill, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(widget->fill, lv_color_black(), 0);
    lv_obj_set_style_border_width(widget->fill, 0, 0);
    lv_obj_set_style_radius(widget->fill, 0, 0);
    lv_obj_set_style_pad_all(widget->fill, 0, 0);
    lv_obj_clear_flag(widget->fill, LV_OBJ_FLAG_SCROLLABLE);

    widget->cap = lv_obj_create(widget->root);
    lv_obj_set_pos(widget->cap, BATTERY_ICON_X + BATTERY_ICON_W, BATTERY_ICON_Y + 2);
    lv_obj_set_size(widget->cap, BATTERY_CAP_W, BATTERY_CAP_H);
    lv_obj_set_style_bg_opa(widget->cap, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(widget->cap, lv_color_black(), 0);
    lv_obj_set_style_border_width(widget->cap, 0, 0);
    lv_obj_set_style_radius(widget->cap, 0, 0);
    lv_obj_set_style_pad_all(widget->cap, 0, 0);
    lv_obj_clear_flag(widget->cap, LV_OBJ_FLAG_SCROLLABLE);

    widget->percent = lv_label_create(widget->root);
    lv_obj_set_pos(widget->percent, 32, 0);
    lv_obj_set_width(widget->percent, width - 32);
    style_text(widget->percent, LV_TEXT_ALIGN_LEFT);
    lv_label_set_text(widget->percent, "--%");

    update_fill(widget, 0U, false);
    return 0;
}

void zmk_insight_display_widget_battery_status_update(
    struct zmk_insight_display_widget_battery_status *widget, uint8_t battery, bool valid) {
    if (widget == NULL || widget->root == NULL) {
        return;
    }

    if (battery > 100U) {
        battery = 100U;
    }

    if (!valid) {
        lv_label_set_text(widget->percent, "--%");
        update_fill(widget, 0U, false);
        return;
    }

    snprintf(widget->text, sizeof(widget->text), "%u%%", battery);
    lv_label_set_text(widget->percent, widget->text);
    update_fill(widget, battery, true);
}
