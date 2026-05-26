#include <stdbool.h>
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
#include <zmk_insight_display/state.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *display_dev;
static bool display_ready;
static uint8_t font_height = 8;

static struct k_work_delayable render_work;

static const char *output_token(const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_OUTPUT_VALID) == 0U) {
        return "--";
    }

    return state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB ? "U" : "B";
}

static const char *ble_token(const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_BLE_VALID) == 0U) {
        return "-";
    }

    switch (state->ble_state) {
    case ZMK_INSIGHT_DISPLAY_BLE_STATE_CONNECTED:
        return "C";
    case ZMK_INSIGHT_DISPLAY_BLE_STATE_ADVERTISING:
        return "A";
    default:
        return "-";
    }
}

static void format_top_row(char *buf, size_t len, const struct zmk_insight_display_state *state) {
    if ((state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LAYER_VALID) == 0U) {
        snprintf(buf, len, "%s %s --", output_token(state), ble_token(state));
        return;
    }

    if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_BLE &&
        (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID) != 0U) {
        snprintf(buf, len, "%s %s BT%u L%u", output_token(state), ble_token(state),
                 state->profile_index + 1, state->layer);
    } else {
        snprintf(buf, len, "%s %s L%u", output_token(state), ble_token(state), state->layer);
    }
}

static void battery_text(char *buf, size_t len, uint8_t battery, bool valid) {
    if (!valid) {
        snprintf(buf, len, "--%%");
        return;
    }

    snprintf(buf, len, "%u%%", battery);
}

static void format_battery_row(char *buf, size_t len, const struct zmk_insight_display_state *state) {
    const bool left_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID) != 0U;
    const bool right_valid = (state->flags & ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID) != 0U;

    if (zmk_insight_display_screen_mode_kind() == ZMK_INSIGHT_DISPLAY_SCREEN_MODE_PAIRED) {
        const bool local_is_left = zmk_insight_display_local_side() == ZMK_INSIGHT_DISPLAY_SIDE_LEFT;
        battery_text(buf, len, local_is_left ? state->left_battery : state->right_battery,
                     local_is_left ? left_valid : right_valid);
        return;
    }

    char left[8];
    char right[8];
    battery_text(left, sizeof(left), state->left_battery, left_valid);
    battery_text(right, sizeof(right), state->right_battery, right_valid);
    snprintf(buf, len, "L:%s R:%s", left, right);
}

static void render_state_to_display(const struct zmk_insight_display_state *state) {
    if (!display_ready) {
        return;
    }

    char top[24];
    char battery[24];

    const bool representative_ready = zmk_insight_display_runtime_ready();

    if (!representative_ready) {
        snprintf(top, sizeof(top), "INITIALIZING...");
        snprintf(battery, sizeof(battery), "PLEASE WAIT");
    } else {
        const bool local_battery_valid = zmk_insight_display_local_battery_valid();
        format_top_row(top, sizeof(top), state);
        if (state->output == ZMK_INSIGHT_DISPLAY_TRANSPORT_USB && !local_battery_valid) {
            snprintf(battery, sizeof(battery), "USB POWER");
        } else {
            format_battery_row(battery, sizeof(battery), state);
        }
    }

    cfb_framebuffer_clear(display_dev, false);
    (void)cfb_print(display_dev, top, 0, 0);
    (void)cfb_print(display_dev, battery, 0,
                    zmk_insight_display_layout_kind() == ZMK_INSIGHT_DISPLAY_LAYOUT_128X64
                        ? font_height + 8
                        : font_height);
    (void)cfb_framebuffer_finalize(display_dev);
}

static void render_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    render_state_to_display(zmk_insight_display_state_ptr());
    (void)k_work_schedule(&render_work, K_MSEC(1000));
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
    uint8_t font_width;

    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        return 0;
    }

    if (display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10) != 0 &&
        display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO01) != 0) {
        return 0;
    }

    if (cfb_framebuffer_init(display_dev) != 0) {
        return 0;
    }

    if (cfb_get_font_size(display_dev, 0, &font_width, &font_height) != 0) {
        font_height = 8;
    } else {
        (void)cfb_framebuffer_set_font(display_dev, 0);
    }

    (void)cfb_framebuffer_clear(display_dev, true);
    (void)cfb_framebuffer_finalize(display_dev);
    k_msleep(40);
    (void)cfb_framebuffer_clear(display_dev, true);
    (void)display_blanking_off(display_dev);

    display_ready = true;
    k_work_init_delayable(&render_work, render_work_handler);
    render_state_to_display(zmk_insight_display_state_ptr());
    (void)k_work_schedule(&render_work, K_MSEC(250));

    return 0;
}

SYS_INIT(zmk_insight_display_display_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
