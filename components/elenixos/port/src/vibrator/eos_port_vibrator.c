#include <stdint.h>

#include "esp_log.h"

#include "eos_dev_vibrator.h"

static const char *TAG = "eos_vibrator";

static void vibrator_on(uint8_t strength)
{
    ESP_LOGI(TAG, "Vibrator on: %u", strength);
}

static void vibrator_off(void)
{
    ESP_LOGI(TAG, "Vibrator off");
}

static const eos_dev_vibrator_ops_t s_vibrator_ops = {
    .on = vibrator_on,
    .off = vibrator_off,
};

void eos_port_vibrator_init(void)
{
    eos_dev_vibrator_register(&s_vibrator_ops);
}
