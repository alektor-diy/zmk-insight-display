#pragma once

#include <zmk_insight_display/state.h>

void zmk_insight_display_sync_service_notify(void);
void zmk_insight_display_sync_mark_unsynced(void);
uint8_t zmk_insight_display_local_battery_percent(void);
