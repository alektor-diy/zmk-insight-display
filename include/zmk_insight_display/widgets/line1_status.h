#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_insight_display_widget_line1_status {
    sys_snode_t node;
    lv_obj_t *obj;
    char text[24];
};

int zmk_insight_display_widget_line1_status_init(
    struct zmk_insight_display_widget_line1_status *widget, lv_obj_t *parent, lv_coord_t x,
    lv_coord_t y, lv_coord_t width);

lv_obj_t *zmk_insight_display_widget_line1_status_obj(
    struct zmk_insight_display_widget_line1_status *widget);
