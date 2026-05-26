#include <stdbool.h>

#include <zmk_insight_display/private.h>

static bool runtime_ready;

bool zmk_insight_display_runtime_ready(void) { return runtime_ready; }

void zmk_insight_display_set_runtime_ready(bool ready) { runtime_ready = ready; }
