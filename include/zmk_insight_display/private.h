#pragma once

#include <stdbool.h>

#include <zmk_insight_display/state.h>

void zmk_insight_display_sync_service_notify(void);
void zmk_insight_display_sync_mark_unsynced(void);
uint8_t zmk_insight_display_local_battery_percent(void);
bool zmk_insight_display_local_battery_valid(void);
bool zmk_insight_display_runtime_ready(void);
void zmk_insight_display_set_runtime_ready(bool ready);
