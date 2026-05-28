#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_insight_display_widget_layer_status {
    sys_snode_t node;
    lv_obj_t *obj;
    char text[8];
};

int zmk_insight_display_widget_layer_status_init(
    struct zmk_insight_display_widget_layer_status *widget, lv_obj_t *parent);

lv_obj_t *zmk_insight_display_widget_layer_status_obj(
    struct zmk_insight_display_widget_layer_status *widget);
