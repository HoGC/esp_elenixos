#include "esp_log.h"

#include "eos_dev_power.h"

static const char *TAG = "eos_power";

static int power_set(dev_power_state_t state)
{
    ESP_LOGI(TAG, "Power state: %d", state);
    return 0;
}

static const eos_dev_power_ops_t s_power_ops = {
    .set_power = power_set,
};

void eos_port_power_init(void)
{
    eos_dev_power_register(&s_power_ops);
}
