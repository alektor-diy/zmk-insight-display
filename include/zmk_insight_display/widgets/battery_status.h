#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_insight_display_widget_battery_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *canvas;
    lv_obj_t *label;
    char side_label;
    char text[12];
    lv_color_t canvas_buffer[5 * 8];
};

int zmk_insight_display_widget_battery_status_init(
    struct zmk_insight_display_widget_battery_status *widget, lv_obj_t *parent, lv_coord_t x,
    lv_coord_t y, lv_coord_t width, char side_label);

void zmk_insight_display_widget_battery_status_update(
    struct zmk_insight_display_widget_battery_status *widget, uint8_t battery, bool valid);
