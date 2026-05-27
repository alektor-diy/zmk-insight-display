#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk_insight_display/config.h>
#include <zmk_insight_display/events/state_changed.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *display_dev;
static bool display_ready;
static uint8_t font_width = 8;
static uint8_t font_height = 8;
static uint16_t display_width = 128;
static uint16_t display_height = 32;
static uint8_t anim_phase;

static struct k_work_delayable render_work;

static const uint32_t cat_frame_a[16] = {
    0x018180, 0x03C3C0, 0x07E7E0, 0x0FFFF0, 0x1E7E78, 0x3C3C3C, 0x387C1C, 0x70C60E,
    0x718E8E, 0x719E8E, 0x30FC0C, 0x187830, 0x0C0030, 0x0E0660, 0x07FFC0, 0x01FF00,
};

static const uint32_t cat_frame_b[16] = {
    0x018180, 0x03C3C0, 0x07E7E0, 0x0FFFF0, 0x1E7E78, 0x3C3C3C, 0x38FC1C, 0x718E8E,
    0x70C60E, 0x719E8E, 0x30FC0C, 0x187830, 0x0C1830, 0x0E3C60, 0x07FFC0, 0x01FF00,
};

static const char *output_label(const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_OUTPUT_VALID) == 0U) {
        return "--";
    }

    return state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB ? "USB" : "BLE";
}

static const char *ble_label(const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_BLE_VALID) == 0U) {
        return "---";
    }

    switch (state->ble_state) {
    case ZMK_INSIGHT_DISPLAY_BLE_STATE_CONNECTED:
        return "CON";
    case ZMK_INSIGHT_DISPLAY_BLE_STATE_ADVERTISING:
        return "ADV";
    default:
        return "---";
    }
}

static void draw_point(uint16_t x, uint16_t y) {
    const struct cfb_position pos = {.x = x, .y = y};
    (void)cfb_draw_point(display_dev, &pos);
}

static void draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    const struct cfb_position start = {.x = x0, .y = y0};
    const struct cfb_position end = {.x = x1, .y = y1};
    (void)cfb_draw_line(display_dev, &start, &end);
}

static void draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    const struct cfb_position start = {.x = x, .y = y};
    const struct cfb_position end = {.x = x + width - 1U, .y = y + height - 1U};
    (void)cfb_draw_rect(display_dev, &start, &end);
}

static void fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    for (uint16_t row = 0; row < height; row++) {
        draw_line(x, y + row, x + width - 1U, y + row);
    }
}

static void draw_bitmap(uint16_t x, uint16_t y, uint8_t width, uint8_t height,
                        const uint32_t *rows) {
    for (uint8_t row = 0; row < height; row++) {
        for (uint8_t col = 0; col < width; col++) {
            if ((rows[row] & (1UL << (width - 1U - col))) != 0U) {
                draw_point(x + col, y + row);
            }
        }
    }
}

static uint16_t draw_chip(uint16_t x, uint16_t y, const char *text, bool invert) {
    const size_t len = strlen(text);
    const uint16_t width = (uint16_t)(len * font_width) + 6U;
    const uint16_t height = font_height + 2U;

    (void)cfb_print(display_dev, text, x + 3U, y + 1U);
    if (invert) {
        (void)cfb_invert_area(display_dev, x, y, width, height);
    } else {
        draw_rect(x, y, width, height);
    }

    return width;
}

static void draw_transport_icon(uint16_t x, uint16_t y,
                                const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_OUTPUT_VALID) == 0U) {
        draw_rect(x, y, 8U, 8U);
        return;
    }

    if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB) {
        draw_line(x + 4U, y + 1U, x + 4U, y + 6U);
        draw_line(x + 4U, y + 1U, x + 2U, y + 3U);
        draw_line(x + 4U, y + 1U, x + 6U, y + 3U);
        draw_line(x + 2U, y + 3U, x + 2U, y + 1U);
        draw_line(x + 6U, y + 3U, x + 6U, y + 1U);
        draw_rect(x + 3U, y + 6U, 3U, 2U);
        return;
    }

    draw_line(x + 2U, y + 1U, x + 6U, y + 3U);
    draw_line(x + 6U, y + 3U, x + 2U, y + 7U);
    draw_line(x + 2U, y + 7U, x + 5U, y + 4U);
    draw_line(x + 5U, y + 4U, x + 2U, y + 1U);
    draw_line(x + 3U, y + 2U, x + 5U, y + 4U);
    draw_line(x + 5U, y + 4U, x + 3U, y + 6U);
}

static void draw_ble_icon(uint16_t x, uint16_t y, const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_BLE_VALID) == 0U) {
        draw_rect(x, y, 8U, 8U);
        return;
    }

    if (state->ble_state == ZMK_INSIGHT_DISPLAY_BLE_STATE_CONNECTED) {
        draw_rect(x, y, 8U, 8U);
        draw_line(x + 2U, y + 4U, x + 3U, y + 5U);
        draw_line(x + 3U, y + 5U, x + 6U, y + 2U);
        return;
    }

    draw_point(x + 2U, y + 4U);
    draw_point(x + 4U, y + 4U);
    draw_point(x + 6U, y + 4U);
    draw_line(x + 2U, y + 2U, x + 4U, y + 1U);
    draw_line(x + 4U, y + 1U, x + 6U, y + 2U);
    draw_line(x + 2U, y + 6U, x + 4U, y + 7U);
    draw_line(x + 4U, y + 7U, x + 6U, y + 6U);
}

static void draw_battery_icon(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                              uint8_t percent, bool valid) {
    const uint16_t body_width = width - 2U;
    const uint16_t nub_height = height / 2U;
    uint16_t fill_width = 0U;

    draw_rect(x, y, body_width, height);
    fill_rect(x + body_width, y + (height - nub_height) / 2U, 2U, nub_height);

    if (!valid) {
        draw_line(x + 1U, y + 1U, x + body_width - 2U, y + height - 2U);
        draw_line(x + body_width - 2U, y + 1U, x + 1U, y + height - 2U);
        return;
    }

    if (percent > 100U) {
        percent = 100U;
    }

    fill_width = (uint16_t)(((body_width - 2U) * percent) / 100U);
    if (fill_width > 0U) {
        fill_rect(x + 1U, y + 1U, fill_width, height - 2U);
    }
}

static void battery_value_text(char *buf, size_t len, uint8_t battery, bool valid) {
    if (!valid) {
        snprintf(buf, len, "--");
        return;
    }

    snprintf(buf, len, "%u", battery);
}

static void draw_battery_pair_compact(const struct zmk_insight_display_state *state, uint16_t x,
                                      uint16_t y) {
    const bool left_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID) != 0U;
    const bool right_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID) != 0U;
    char left[8];
    char right[8];

    battery_value_text(left, sizeof(left), state->left_battery, left_valid);
    battery_value_text(right, sizeof(right), state->right_battery, right_valid);

    (void)cfb_print(display_dev, "L", x, y);
    draw_battery_icon(x + 9U, y + 1U, 14U, 7U, state->left_battery, left_valid);
    (void)cfb_print(display_dev, left, x + 27U, y);

    (void)cfb_print(display_dev, "R", x + 46U, y);
    draw_battery_icon(x + 55U, y + 1U, 14U, 7U, state->right_battery, right_valid);
    (void)cfb_print(display_dev, right, x + 73U, y);
}

static void draw_local_battery_card(const struct zmk_insight_display_state *state, uint16_t x,
                                    uint16_t y, uint16_t width) {
    const bool local_is_left = zmk_insight_display_local_side() == ZMK_INSIGHT_DISPLAY_SIDE_LEFT;
    const bool valid = local_is_left
                           ? ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID) != 0U)
                           : ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID) != 0U);
    const uint8_t battery = local_is_left ? state->left_battery : state->right_battery;
    char value[8];

    battery_value_text(value, sizeof(value), battery, valid);
    draw_rect(x, y, width, 13U);
    (void)cfb_print(display_dev, local_is_left ? "LEFT" : "RIGHT", x + 3U, y + 2U);
    draw_battery_icon(x + 34U, y + 3U, width - 38U, 7U, battery, valid);
    (void)cfb_print(display_dev, value, x + width - 18U, y + 2U);
}

static void draw_usb_power_card(uint16_t x, uint16_t y, uint16_t width) {
    draw_rect(x, y, width, 13U);
    draw_transport_icon(x + 4U, y + 2U,
                        &(struct zmk_insight_display_state){
                            .flags = ZMK_INSIGHT_DISPLAY_FLAG_OUTPUT_VALID,
                            .output = ZMK_INSIGHT_DISPLAY_TRANSPORT_USB,
                        });
    (void)cfb_print(display_dev, "USB POWER", x + 16U, y + 2U);
}

static void draw_mascot(uint16_t x, uint16_t y) {
    draw_bitmap(x, y, 24U, 16U, (anim_phase & 1U) == 0U ? cat_frame_a : cat_frame_b);
}

static void render_128x32(const struct zmk_insight_display_state *state, bool representative_ready) {
    uint16_t x = 0U;
    char profile[8];
    const bool local_battery_valid = zmk_insight_display_local_battery_valid();

    if (!representative_ready) {
        draw_chip(0U, 0U, "BOOT", true);
        draw_chip(37U, 0U, "SYNC", false);
        (void)cfb_print(display_dev, "INITIALIZING", 0U, 14U);
        draw_mascot(96U, 14U);
        return;
    }

    x += draw_chip(x, 0U, output_label(state), true) + 4U;
    draw_transport_icon(x - 12U, 1U, state);

    x += draw_chip(x, 0U, ble_label(state), false) + 4U;
    draw_ble_icon(x - 12U, 1U, state);

    if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_BLE &&
        (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID) != 0U) {
        snprintf(profile, sizeof(profile), "P%u", state->profile_index + 1U);
        x += draw_chip(x, 0U, profile, false) + 4U;
    }

    snprintf(profile, sizeof(profile), "L%u", state->layer);
    (void)draw_chip(display_width - 22U, 0U, profile, true);

    if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB && !local_battery_valid) {
        draw_usb_power_card(0U, 14U, 82U);
    } else if (zmk_insight_display_screen_mode_kind() == ZMK_INSIGHT_DISPLAY_SCREEN_MODE_PAIRED) {
        draw_local_battery_card(state, 0U, 14U, 82U);
    } else {
        draw_battery_pair_compact(state, 0U, 16U);
    }

    draw_mascot(96U, 14U);
}

static void render_128x64(const struct zmk_insight_display_state *state, bool representative_ready) {
    char layer[8];
    char profile[8];
    const bool local_battery_valid = zmk_insight_display_local_battery_valid();

    if (!representative_ready) {
        draw_chip(0U, 0U, "INITIALIZING", true);
        draw_chip(89U, 0U, "WAIT", false);
        (void)cfb_print(display_dev, "STATE LINKING", 0U, 18U);
        (void)cfb_print(display_dev, "PLEASE WAIT", 0U, 30U);
        draw_mascot(96U, 40U);
        return;
    }

    draw_chip(0U, 0U, output_label(state), true);
    draw_transport_icon(20U, 1U, state);

    draw_chip(28U, 0U, ble_label(state), false);
    draw_ble_icon(48U, 1U, state);

    if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_BLE &&
        (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID) != 0U) {
        snprintf(profile, sizeof(profile), "PROFILE %u", state->profile_index + 1U);
        draw_chip(58U, 0U, profile, false);
    }

    snprintf(layer, sizeof(layer), "L%u", state->layer);
    draw_chip(104U, 0U, layer, true);

    if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB && !local_battery_valid) {
        draw_usb_power_card(0U, 18U, 88U);
    } else if (zmk_insight_display_screen_mode_kind() == ZMK_INSIGHT_DISPLAY_SCREEN_MODE_PAIRED) {
        draw_local_battery_card(state, 0U, 18U, 88U);
    } else {
        draw_battery_pair_compact(state, 0U, 18U);
    }

    if (zmk_insight_display_screen_mode_kind() == ZMK_INSIGHT_DISPLAY_SCREEN_MODE_SINGLE) {
        draw_local_battery_card(state, 0U, 36U, 88U);
    } else {
        (void)cfb_print(display_dev, "SYNCED SPLIT VIEW", 0U, 38U);
    }

    draw_mascot(96U, 40U);
}

static void render_state_to_display(const struct zmk_insight_display_state *state) {
    if (!display_ready) {
        return;
    }

    cfb_framebuffer_clear(display_dev, false);

    if (display_width >= 128U && display_height > 32U) {
        render_128x64(state, zmk_insight_display_runtime_ready());
    } else {
        render_128x32(state, zmk_insight_display_runtime_ready());
    }

    (void)cfb_framebuffer_finalize(display_dev);
}

static void render_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    anim_phase++;
    render_state_to_display(zmk_insight_display_state_ptr());
    (void)k_work_schedule(&render_work, K_MSEC(450));
}

static int display_listener(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    if (display_ready) {
        (void)k_work_reschedule(&render_work, K_NO_WAIT);
    }
    return 0;
}

ZMK_LISTENER(zmk_insight_display_display, display_listener);
ZMK_SUBSCRIPTION(zmk_insight_display_display, zmk_insight_display_state_changed);

static int zmk_insight_display_display_init(void) {
    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        return 0;
    }

    k_work_init_delayable(&render_work, render_work_handler);

    if (display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10) != 0 &&
        display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO01) != 0) {
        return 0;
    }

    if (cfb_framebuffer_init(display_dev) != 0) {
        return 0;
    }

    if (cfb_get_font_size(display_dev, 0, &font_width, &font_height) != 0) {
        font_width = 8;
        font_height = 8;
    } else {
        (void)cfb_framebuffer_set_font(display_dev, 0);
    }

    display_width = (uint16_t)cfb_get_display_parameter(display_dev, CFB_DISPLAY_WIDTH);
    display_height = (uint16_t)cfb_get_display_parameter(display_dev, CFB_DISPLAY_HEIGH);

    (void)cfb_framebuffer_clear(display_dev, true);
    (void)cfb_framebuffer_finalize(display_dev);
    k_msleep(40);
    (void)cfb_framebuffer_clear(display_dev, true);
    (void)display_blanking_off(display_dev);

    display_ready = true;
    render_state_to_display(zmk_insight_display_state_ptr());
    (void)k_work_schedule(&render_work, K_MSEC(250));

    return 0;
}

SYS_INIT(zmk_insight_display_display_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
