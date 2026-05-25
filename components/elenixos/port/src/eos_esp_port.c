#include "eos_esp_port.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wear_levelling.h"

#include "eos_dispatcher.h"

static const char *TAG = "eos_port";

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

static esp_err_t mount_flash_fatfs(void)
{
    if (s_wl_handle != WL_INVALID_HANDLE) {
        return ESP_OK;
    }

    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
        .use_one_fat = false,
    };

    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl("/flash", "storage", &mount_config, &s_wl_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Flash FATFS mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

esp_err_t eos_esp_port_init(void)
{
    esp_err_t ret = mount_flash_fatfs();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = eos_port_display_init();
    if (ret != ESP_OK) {
        return ret;
    }

    eos_port_power_init();
    eos_port_time_init();
    eos_port_battery_init();
    eos_port_vibrator_init();
    eos_port_audio_init();
    eos_port_sensor_init();

    return ESP_OK;
}

void eos_esp_port_tick(void)
{
    eos_dispatch_tick();
}

void eos_delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void eos_cpu_reset(void)
{
    esp_restart();
}

void eos_bluetooth_enable(void)
{
    ESP_LOGI(TAG, "Bluetooth enable requested");
}

void eos_bluetooth_disable(void)
{
    ESP_LOGI(TAG, "Bluetooth disable requested");
}

void eos_locate_phone(void)
{
    ESP_LOGI(TAG, "Locate phone requested");
}

void eos_speaker_set_volume(uint8_t volume)
{
    ESP_LOGI(TAG, "Speaker volume set: %u", volume);
}
