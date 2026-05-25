#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "jerryscript-port.h"

static const char *TAG = "jerry_port";

double jerry_port_current_time(void)
{
    return (double)(esp_timer_get_time() / 1000);
}

int32_t jerry_port_local_tza(double unix_ms)
{
    (void)unix_ms;
    return 0;
}

void jerry_port_fatal(jerry_fatal_code_t code)
{
    ESP_LOGE(TAG, "Fatal error: %d", code);
    abort();
}
