#include <lvgl.h>

#include <zmk_insight_display/widgets/battery_status.h>
#include <zmk_insight_display/widgets/layer_status.h>
#include <zmk_insight_display/widgets/line1_status.h>
#include <zmk_insight_display/widgets/output_status.h>

static struct zmk_insight_display_widget_line1_status line1_status_widget;
static struct zmk_insight_display_widget_output_status output_status_widget;
static struct zmk_insight_display_widget_battery_status left_battery_widget;
static struct zmk_insight_display_widget_battery_status right_battery_widget;
static struct zmk_insight_display_widget_layer_status layer_status_widget;

static lv_style_t global_style;
static bool style_initialized;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_set_size(screen, 128, 32);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    if (!style_initialized) {
        lv_style_init(&global_style);
        lv_style_set_text_font(&global_style, &lv_font_unscii_8);
        lv_style_set_text_letter_space(&global_style, 1);
        lv_style_set_text_line_space(&global_style, 1);
        style_initialized = true;
    }

    lv_obj_add_style(screen, &global_style, LV_PART_MAIN);

    zmk_insight_display_widget_line1_status_init(&line1_status_widget, screen, 0, 11, 128);

    zmk_insight_display_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_insight_display_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0,
                 0);

    zmk_insight_display_widget_battery_status_init(&left_battery_widget, screen, 0, 18, 64, 'L');
    zmk_insight_display_widget_battery_status_init(&right_battery_widget, screen, 64, 0, 64, 'R');

    zmk_insight_display_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_insight_display_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_RIGHT, 0,
                 0);

    return screen;
}
