#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>
#include <zmk_insight_display/uuid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct bt_uuid_128 insight_service_uuid =
    BT_UUID_INIT_128(ZMK_INSIGHT_DISPLAY_SERVICE_UUID);
static const struct bt_uuid_128 insight_state_uuid =
    BT_UUID_INIT_128(ZMK_INSIGHT_DISPLAY_STATE_UUID);

static void insight_state_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    ARG_UNUSED(attr);
    ARG_UNUSED(value);
}

static ssize_t read_insight_state(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                                  uint16_t len, uint16_t offset) {
    const struct zmk_insight_display_state *state = zmk_insight_display_state_ptr();
    return bt_gatt_attr_read(conn, attr, buf, len, offset, state, sizeof(*state));
}

BT_GATT_SERVICE_DEFINE(
    zmk_insight_display_svc, BT_GATT_PRIMARY_SERVICE(&insight_service_uuid.uuid),
    BT_GATT_CHARACTERISTIC(&insight_state_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ_ENCRYPT, read_insight_state, NULL, NULL),
    BT_GATT_CCC(insight_state_ccc_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT), );

void zmk_insight_display_sync_service_notify(void) {
    const struct zmk_insight_display_state *state = zmk_insight_display_state_ptr();
    int err = bt_gatt_notify(NULL, &zmk_insight_display_svc.attrs[2], state, sizeof(*state));
    if (err < 0) {
        LOG_DBG("Insight display notify skipped (%d)", err);
    }
}

void zmk_insight_display_sync_mark_unsynced(void) {}
