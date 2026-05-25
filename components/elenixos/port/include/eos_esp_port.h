#pragma once

#include "esp_err.h"

typedef struct lv_display_t lv_display_t;

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t eos_esp_port_init(void);
void eos_esp_port_tick(void);
esp_err_t eos_port_display_init(void);
lv_display_t *eos_port_display_get_physical(void);
void eos_port_power_init(void);
void eos_port_time_init(void);
void eos_port_battery_init(void);
void eos_port_vibrator_init(void);
void eos_port_audio_init(void);
void eos_port_sensor_init(void);

#ifdef __cplusplus
}
#endif
