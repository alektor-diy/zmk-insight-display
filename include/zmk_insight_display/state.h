#pragma once

#include <zephyr/sys/util.h>

#include <stdbool.h>
#include <stdint.h>

enum zmk_insight_display_transport {
    ZMK_INSIGHT_DISPLAY_TRANSPORT_USB = 0,
    ZMK_INSIGHT_DISPLAY_TRANSPORT_BLE = 1,
};

enum zmk_insight_display_ble_state {
    ZMK_INSIGHT_DISPLAY_BLE_STATE_DISCONNECTED = 0,
    ZMK_INSIGHT_DISPLAY_BLE_STATE_ADVERTISING = 1,
    ZMK_INSIGHT_DISPLAY_BLE_STATE_CONNECTED = 2,
};

enum zmk_insight_display_flags {
    ZMK_INSIGHT_DISPLAY_FLAG_LAYER_VALID = BIT(0),
    ZMK_INSIGHT_DISPLAY_FLAG_OUTPUT_VALID = BIT(1),
    ZMK_INSIGHT_DISPLAY_FLAG_BLE_VALID = BIT(2),
    ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID = BIT(3),
    ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID = BIT(4),
    ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID = BIT(5),
    ZMK_INSIGHT_DISPLAY_FLAG_SYNCED = BIT(6),
};

struct zmk_insight_display_state {
    uint8_t version;
    uint8_t flags;
    uint8_t output;
    uint8_t ble_state;
    uint8_t layer;
    uint8_t profile_index;
    uint8_t left_battery;
    uint8_t right_battery;
} __packed;

const struct zmk_insight_display_state *zmk_insight_display_state_ptr(void);
bool zmk_insight_display_state_get(struct zmk_insight_display_state *out);
bool zmk_insight_display_state_set(const struct zmk_insight_display_state *next);
void zmk_insight_display_state_reset(void);
