#define DT_DRV_COMPAT zmk_insight_display

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#include <zmk_insight_display/config.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
             "zmk,insight-display requires exactly one enabled node per firmware image");

enum zmk_insight_display_side zmk_insight_display_local_side(void) {
    return (enum zmk_insight_display_side)DT_INST_ENUM_IDX(0, side);
}

enum zmk_insight_display_layout zmk_insight_display_layout_kind(void) {
    return (enum zmk_insight_display_layout)DT_INST_ENUM_IDX_OR(0, layout, 0);
}

enum zmk_insight_display_screen_mode zmk_insight_display_screen_mode_kind(void) {
    return (enum zmk_insight_display_screen_mode)DT_INST_ENUM_IDX_OR(0, screen_mode, 0);
}

bool zmk_insight_display_has_local_display(void) { return DT_INST_NODE_HAS_PROP(0, display); }
