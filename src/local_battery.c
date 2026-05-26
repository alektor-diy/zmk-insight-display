#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <zmk_insight_display/private.h>

#ifdef CONFIG_ZMK_NON_LIPO_LOW_MV
#define ZMK_INSIGHT_DISPLAY_BATTERY_VALID_MV CONFIG_ZMK_NON_LIPO_LOW_MV
#else
#define ZMK_INSIGHT_DISPLAY_BATTERY_VALID_MV 1
#endif

static bool battery_valid;
static uint8_t battery_percent_cached;

#if DT_HAS_CHOSEN(zmk_battery)
static const struct device *const battery_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_battery));
#endif

uint8_t zmk_insight_display_local_battery_percent(void) {
#if DT_HAS_CHOSEN(zmk_battery)
    struct sensor_value value;
    struct sensor_value voltage;
    int32_t millivolts;

    if (!device_is_ready(battery_dev)) {
        battery_valid = false;
        battery_percent_cached = 0U;
        return battery_percent_cached;
    }

    if (sensor_sample_fetch(battery_dev) != 0) {
        battery_valid = false;
        battery_percent_cached = 0U;
        return battery_percent_cached;
    }

    if (sensor_channel_get(battery_dev, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &value) != 0) {
        battery_valid = false;
        battery_percent_cached = 0U;
        return battery_percent_cached;
    }

    if (value.val1 < 0) {
        battery_valid = false;
        battery_percent_cached = 0U;
        return battery_percent_cached;
    }

    if (sensor_channel_get(battery_dev, SENSOR_CHAN_GAUGE_VOLTAGE, &voltage) == 0) {
        millivolts = (voltage.val1 * 1000) + (voltage.val2 / 1000);
        battery_valid = millivolts >= ZMK_INSIGHT_DISPLAY_BATTERY_VALID_MV;
    } else {
        battery_valid = true;
    }

    if (value.val1 > 100) {
        battery_percent_cached = 100U;
        return battery_percent_cached;
    }

    battery_percent_cached = (uint8_t)value.val1;
    return battery_percent_cached;
#else
    battery_valid = false;
    battery_percent_cached = 0U;
    return battery_percent_cached;
#endif
}

bool zmk_insight_display_local_battery_valid(void) {
    (void)zmk_insight_display_local_battery_percent();
    return battery_valid;
}
