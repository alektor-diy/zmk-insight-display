#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_insight_display_widget_output_status {
    sys_snode_t node;
    lv_obj_t *obj;
};

int zmk_insight_display_widget_output_status_init(
    struct zmk_insight_display_widget_output_status *widget, lv_obj_t *parent);

lv_obj_t *zmk_insight_display_widget_output_status_obj(
    struct zmk_insight_display_widget_output_status *widget);
