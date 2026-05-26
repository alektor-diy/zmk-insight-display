#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/split/central.h>

#include <zmk_insight_display/config.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

BUILD_ASSERT(CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS == 1,
             "Insight display currently supports exactly one split peripheral");

static uint8_t last_remote_battery;
static bool remote_battery_valid;
static struct k_work_delayable delayed_publish_work;

static void assign_battery_pair(struct zmk_insight_display_state *state, uint8_t local_battery,
                                uint8_t remote_battery, bool remote_valid) {
    if (zmk_insight_display_local_side() == ZMK_INSIGHT_DISPLAY_SIDE_LEFT) {
        state->left_battery = local_battery;
        state->flags |= ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID;

        state->right_battery = remote_battery;
        if (remote_valid) {
            state->flags |= ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID;
        }
    } else {
        state->right_battery = local_battery;
        state->flags |= ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID;

        state->left_battery = remote_battery;
        if (remote_valid) {
            state->flags |= ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID;
        }
    }
}

static struct zmk_insight_display_state build_state(void) {
    struct zmk_insight_display_state state = {
        .version = CONFIG_ZMK_INSIGHT_DISPLAY_SYNC_VERSION,
        .flags = ZMK_INSIGHT_DISPLAY_FLAG_LAYER_VALID | ZMK_INSIGHT_DISPLAY_FLAG_OUTPUT_VALID |
                 ZMK_INSIGHT_DISPLAY_FLAG_BLE_VALID | ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID |
                 ZMK_INSIGHT_DISPLAY_FLAG_SYNCED,
        .output = (uint8_t)zmk_endpoints_selected().transport,
        .layer = zmk_keymap_highest_layer_active(),
        .profile_index = zmk_ble_active_profile_index(),
        .ble_state = zmk_ble_active_profile_is_connected()
                         ? ZMK_INSIGHT_DISPLAY_BLE_STATE_CONNECTED
                         : ZMK_INSIGHT_DISPLAY_BLE_STATE_ADVERTISING,
    };

    assign_battery_pair(&state, zmk_insight_display_local_battery_percent(), last_remote_battery,
                        remote_battery_valid);

    return state;
}

static int publish_state(void) {
    struct zmk_insight_display_state next = build_state();
    if (zmk_insight_display_state_set(&next)) {
        zmk_insight_display_sync_service_notify();
    }

    return 0;
}

static void delayed_publish_handler(struct k_work *work) {
    ARG_UNUSED(work);
    zmk_insight_display_set_runtime_ready(true);
    (void)publish_state();
}

static int insight_display_central_listener(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *peripheral_battery =
        as_zmk_peripheral_battery_state_changed(eh);
    if (peripheral_battery != NULL) {
        last_remote_battery = peripheral_battery->state_of_charge;
        remote_battery_valid = peripheral_battery->state_of_charge > 0;
    }

    return publish_state();
}

ZMK_LISTENER(zmk_insight_display_central, insight_display_central_listener);
ZMK_SUBSCRIPTION(zmk_insight_display_central, zmk_battery_state_changed);
ZMK_SUBSCRIPTION(zmk_insight_display_central, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(zmk_insight_display_central, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(zmk_insight_display_central, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(zmk_insight_display_central, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(zmk_insight_display_central, zmk_usb_conn_state_changed);

static int zmk_insight_display_central_init(void) {
    zmk_insight_display_set_runtime_ready(false);
    k_work_init_delayable(&delayed_publish_work, delayed_publish_handler);
    (void)publish_state();
    (void)k_work_schedule(&delayed_publish_work, K_SECONDS(3));
    return 0;
}

SYS_INIT(zmk_insight_display_central_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
