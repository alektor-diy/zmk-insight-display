#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_insight_display_widget_battery_status {
    sys_snode_t node;
    lv_obj_t *obj;
};

int zmk_insight_display_widget_battery_status_init(
    struct zmk_insight_display_widget_battery_status *widget, lv_obj_t *parent);

lv_obj_t *zmk_insight_display_widget_battery_status_obj(
    struct zmk_insight_display_widget_battery_status *widget);
