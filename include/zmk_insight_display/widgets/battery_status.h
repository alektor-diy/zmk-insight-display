#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lvgl.h>

struct zmk_insight_display_widget_battery_status {
    lv_obj_t *root;
    lv_obj_t *side;
    lv_obj_t *icon;
    lv_obj_t *percent;
    char side_label;
    char text[8];
};

int zmk_insight_display_widget_battery_status_init(
    struct zmk_insight_display_widget_battery_status *widget, lv_obj_t *parent, lv_coord_t x,
    lv_coord_t y, lv_coord_t width, char side_label);

void zmk_insight_display_widget_battery_status_update(
    struct zmk_insight_display_widget_battery_status *widget, uint8_t battery, bool valid);
