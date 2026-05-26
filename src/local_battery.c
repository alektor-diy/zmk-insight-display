#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <zmk_insight_display/private.h>

#if DT_HAS_CHOSEN(zmk_battery)
static const struct device *const battery_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_battery));
#endif

uint8_t zmk_insight_display_local_battery_percent(void) {
#if DT_HAS_CHOSEN(zmk_battery)
    struct sensor_value value;

    if (!device_is_ready(battery_dev)) {
        return 0U;
    }

    if (sensor_sample_fetch(battery_dev) != 0) {
        return 0U;
    }

    if (sensor_channel_get(battery_dev, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &value) != 0) {
        return 0U;
    }

    if (value.val1 < 0) {
        return 0U;
    }

    if (value.val1 > 100) {
        return 100U;
    }

    return (uint8_t)value.val1;
#else
    return 0U;
#endif
}
