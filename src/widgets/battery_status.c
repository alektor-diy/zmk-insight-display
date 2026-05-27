#include <stdio.h>

#include <lvgl.h>

#include <zmk_insight_display/widgets/battery_status.h>

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_BATTERY_EMPTY_14X8
#define LV_ATTRIBUTE_IMG_BATTERY_EMPTY_14X8
#endif

#ifndef LV_ATTRIBUTE_IMG_BATTERY_25_14X8
#define LV_ATTRIBUTE_IMG_BATTERY_25_14X8
#endif

#ifndef LV_ATTRIBUTE_IMG_BATTERY_50_14X8
#define LV_ATTRIBUTE_IMG_BATTERY_50_14X8
#endif

#ifndef LV_ATTRIBUTE_IMG_BATTERY_75_14X8
#define LV_ATTRIBUTE_IMG_BATTERY_75_14X8
#endif

#ifndef LV_ATTRIBUTE_IMG_BATTERY_FULL_14X8
#define LV_ATTRIBUTE_IMG_BATTERY_FULL_14X8
#endif

#define BATTERY_ICON_W 14
#define BATTERY_ICON_H 8

static const lv_img_dsc_t battery_empty_14x8 = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = BATTERY_ICON_W,
    .header.h = BATTERY_ICON_H,
    .data_size = sizeof(battery_palette) + sizeof(battery_empty_14x8_map),
    .data = (const uint8_t[]){
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x80,
        0x10, 0x80, 0x1C, 0x80, 0x1C, 0x80, 0x1C, 0x80, 0x1C, 0x80, 0x10, 0xFF,
        0xF0,
    },
};

static const lv_img_dsc_t battery_25_14x8 = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = BATTERY_ICON_W,
    .header.h = BATTERY_ICON_H,
    .data_size = sizeof(battery_palette) + sizeof(battery_25_14x8_map),
    .data = (const uint8_t[]){
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x80,
        0x10, 0xB0, 0x1C, 0xB0, 0x1C, 0xB0, 0x1C, 0xB0, 0x1C, 0x80, 0x10, 0xFF,
        0xF0,
    },
};

static const lv_img_dsc_t battery_50_14x8 = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = BATTERY_ICON_W,
    .header.h = BATTERY_ICON_H,
    .data_size = sizeof(battery_palette) + sizeof(battery_50_14x8_map),
    .data = (const uint8_t[]){
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x80,
        0x10, 0xFC, 0x1C, 0xFC, 0x1C, 0xFC, 0x1C, 0xFC, 0x1C, 0x80, 0x10, 0xFF,
        0xF0,
    },
};

static const lv_img_dsc_t battery_75_14x8 = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = BATTERY_ICON_W,
    .header.h = BATTERY_ICON_H,
    .data_size = sizeof(battery_palette) + sizeof(battery_75_14x8_map),
    .data = (const uint8_t[]){
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x80,
        0x10, 0xFF, 0x9C, 0xFF, 0x9C, 0xFF, 0x9C, 0xFF, 0x9C, 0x80, 0x10, 0xFF,
        0xF0,
    },
};

static const lv_img_dsc_t battery_full_14x8 = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = BATTERY_ICON_W,
    .header.h = BATTERY_ICON_H,
    .data_size = sizeof(battery_palette) + sizeof(battery_full_14x8_map),
    .data = (const uint8_t[]){
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x80,
        0x10, 0xFF, 0xFC, 0xFF, 0xFC, 0xFF, 0xFC, 0xFF, 0xFC, 0x80, 0x10, 0xFF,
        0xF0,
    },
};

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

static const lv_img_dsc_t *battery_image(uint8_t battery, bool valid) {
    if (!valid) {
        return &battery_empty_14x8;
    }

    if (battery >= 90U) {
        return &battery_full_14x8;
    }

    if (battery >= 65U) {
        return &battery_75_14x8;
    }

    if (battery >= 40U) {
        return &battery_50_14x8;
    }

    if (battery >= 15U) {
        return &battery_25_14x8;
    }

    return &battery_empty_14x8;
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

    widget->icon = lv_img_create(widget->root);
    lv_obj_set_pos(widget->icon, 12, 2);
    lv_img_set_src(widget->icon, &battery_empty_14x8);

    widget->percent = lv_label_create(widget->root);
    lv_obj_set_pos(widget->percent, 30, 0);
    lv_obj_set_width(widget->percent, width - 30);
    style_text(widget->percent, LV_TEXT_ALIGN_LEFT);
    lv_label_set_text(widget->percent, "--%");

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

    lv_img_set_src(widget->icon, battery_image(battery, valid));

    if (!valid) {
        lv_label_set_text(widget->percent, "--%");
        return;
    }

    snprintf(widget->text, sizeof(widget->text), "%u%%", battery);
    lv_label_set_text(widget->percent, widget->text);
}
