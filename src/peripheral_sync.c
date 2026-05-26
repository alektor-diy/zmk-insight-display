#include <string.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk_insight_display/config.h>
#include <zmk_insight_display/private.h>
#include <zmk_insight_display/state.h>
#include <zmk_insight_display/uuid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct bt_uuid_128 insight_service_uuid =
    BT_UUID_INIT_128(ZMK_INSIGHT_DISPLAY_SERVICE_UUID);
static const struct bt_uuid_128 insight_state_uuid =
    BT_UUID_INIT_128(ZMK_INSIGHT_DISPLAY_STATE_UUID);

static struct bt_conn *sync_conn;
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;
static struct bt_gatt_read_params read_params;
static uint16_t service_end_handle;
static bool subscribed;

static int apply_remote_state(const void *data, uint16_t length) {
    if (data == NULL || length < sizeof(struct zmk_insight_display_state) ||
        length > CONFIG_ZMK_INSIGHT_DISPLAY_MAX_SERVICE_READ) {
        return -EINVAL;
    }

    struct zmk_insight_display_state next;
    memcpy(&next, data, sizeof(next));
    next.flags |= ZMK_INSIGHT_DISPLAY_FLAG_SYNCED;
    zmk_insight_display_state_set(&next);
    return 0;
}

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                         const void *data, uint16_t length) {
    ARG_UNUSED(conn);

    if (data == NULL) {
        params->value_handle = 0U;
        subscribed = false;
        return BT_GATT_ITER_STOP;
    }

    apply_remote_state(data, length);
    return BT_GATT_ITER_CONTINUE;
}

static uint8_t read_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
                       const void *data, uint16_t length) {
    ARG_UNUSED(conn);
    ARG_UNUSED(params);

    if (err > 0) {
        return BT_GATT_ITER_STOP;
    }

    if (data == NULL) {
        return BT_GATT_ITER_STOP;
    }

    apply_remote_state(data, length);
    return BT_GATT_ITER_STOP;
}

static int start_read(void) {
    if (sync_conn == NULL || subscribe_params.value_handle == 0U) {
        return -ENODEV;
    }

    read_params.func = read_cb;
    read_params.handle_count = 1;
    read_params.single.handle = subscribe_params.value_handle;
    read_params.single.offset = 0;
    return bt_gatt_read(sync_conn, &read_params);
}

static uint8_t discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           struct bt_gatt_discover_params *params) {
    if (attr == NULL) {
        memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    if (params->type == BT_GATT_DISCOVER_PRIMARY) {
        const struct bt_gatt_service_val *service = attr->user_data;

        service_end_handle = service->end_handle;
        discover_params.uuid = &insight_state_uuid.uuid;
        discover_params.start_handle = attr->handle + 1;
        discover_params.end_handle = service_end_handle;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
        discover_params.func = discover_cb;
        bt_gatt_discover(conn, &discover_params);
        return BT_GATT_ITER_STOP;
    }

    if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
        const struct bt_gatt_chrc *chrc = attr->user_data;
        subscribe_params.notify = notify_cb;
        subscribe_params.value = BT_GATT_CCC_NOTIFY;
        subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);
        subscribe_params.disc_params = &discover_params;
        subscribe_params.end_handle = service_end_handle;

        if (bt_gatt_subscribe(conn, &subscribe_params) == 0) {
            subscribed = true;
        }

        start_read();
        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_STOP;
}

static int start_discovery(struct bt_conn *conn) {
    if (conn == NULL || subscribed || subscribe_params.value_handle != 0U) {
        return 0;
    }

    discover_params.uuid = &insight_service_uuid.uuid;
    discover_params.func = discover_cb;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;
    return bt_gatt_discover(conn, &discover_params);
}

static void clear_sync(void) {
    if (sync_conn != NULL) {
        bt_conn_unref(sync_conn);
        sync_conn = NULL;
    }

    memset(&discover_params, 0, sizeof(discover_params));
    memset(&subscribe_params, 0, sizeof(subscribe_params));
    memset(&read_params, 0, sizeof(read_params));
    service_end_handle = 0;
    subscribed = false;
    zmk_insight_display_sync_mark_unsynced();
}

static void connected(struct bt_conn *conn, uint8_t err) {
    struct bt_conn_info info;

    if (err != 0 || bt_conn_get_info(conn, &info) != 0 || info.role != BT_CONN_ROLE_PERIPHERAL) {
        return;
    }

    clear_sync();
    sync_conn = bt_conn_ref(conn);
    if (bt_conn_get_security(sync_conn) >= BT_SECURITY_L2) {
        start_discovery(sync_conn);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    ARG_UNUSED(reason);

    if (conn == sync_conn) {
        clear_sync();
    }
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    if (conn != sync_conn || err != BT_SECURITY_ERR_SUCCESS || level < BT_SECURITY_L2) {
        return;
    }

    start_discovery(conn);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

void zmk_insight_display_sync_mark_unsynced(void) {
    struct zmk_insight_display_state next;
    zmk_insight_display_state_get(&next);
    next.flags &= ~ZMK_INSIGHT_DISPLAY_FLAG_SYNCED;
    next.flags &= ~(ZMK_INSIGHT_DISPLAY_FLAG_LAYER_VALID | ZMK_INSIGHT_DISPLAY_FLAG_OUTPUT_VALID |
                    ZMK_INSIGHT_DISPLAY_FLAG_BLE_VALID | ZMK_INSIGHT_DISPLAY_FLAG_PROFILE_VALID);

    if (zmk_insight_display_local_side() == ZMK_INSIGHT_DISPLAY_SIDE_LEFT) {
        next.left_battery = zmk_insight_display_local_battery_percent();
        next.flags |= ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID;
        next.flags &= ~ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID;
    } else {
        next.right_battery = zmk_insight_display_local_battery_percent();
        next.flags |= ZMK_INSIGHT_DISPLAY_FLAG_RIGHT_BATTERY_VALID;
        next.flags &= ~ZMK_INSIGHT_DISPLAY_FLAG_LEFT_BATTERY_VALID;
    }

    zmk_insight_display_state_set(&next);
}

static int zmk_insight_display_peripheral_sync_init(void) {
    bt_conn_cb_register(&conn_callbacks);
    zmk_insight_display_sync_mark_unsynced();
    return 0;
}

SYS_INIT(zmk_insight_display_peripheral_sync_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
