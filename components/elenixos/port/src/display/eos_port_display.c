#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dev_lcd_touch.h"
#include "esp_board_manager_includes.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"

#include "eos_dev_display.h"

static const char *TAG = "eos_display";

#define LVGL_TICK_PERIOD_MS         5
#define LVGL_TASK_MAX_SLEEP_MS      500
#define LVGL_TASK_MIN_SLEEP_MS      1
#define LVGL_TASK_STACK_SIZE        (10 * 1024)
#define LVGL_TASK_PRIORITY          5
#define LVGL_DRAW_BUF_HEIGHT        100
#define TOUCH_SCROLL_LIMIT_PX       24
#define LCD_RGB_FRAME_BUFFER_COUNT  3

static lv_display_t *s_physical_disp;
static bool s_hw_inited;
static bool s_ops_registered;

static esp_lv_adapter_rotation_t lcd_rotation_from_config(const dev_display_lcd_config_t *lcd_cfg)
{
    if (lcd_cfg->swap_xy) {
        if (lcd_cfg->mirror_x && !lcd_cfg->mirror_y) {
            return ESP_LV_ADAPTER_ROTATE_90;
        }
        if (!lcd_cfg->mirror_x && lcd_cfg->mirror_y) {
            return ESP_LV_ADAPTER_ROTATE_270;
        }
    } else if (lcd_cfg->mirror_x && lcd_cfg->mirror_y) {
        return ESP_LV_ADAPTER_ROTATE_180;
    }

    return ESP_LV_ADAPTER_ROTATE_0;
}

static uint32_t lcd_estimated_refresh_hz(const dev_display_lcd_config_t *lcd_cfg)
{
    const esp_lcd_rgb_panel_config_t *panel_cfg = &lcd_cfg->sub_cfg.rgb.panel_config;
    const esp_lcd_rgb_timing_t *timings = &panel_cfg->timings;
    const uint32_t h_total = timings->h_res + timings->hsync_pulse_width +
                             timings->hsync_back_porch + timings->hsync_front_porch;
    const uint32_t v_total = timings->v_res + timings->vsync_pulse_width +
                             timings->vsync_back_porch + timings->vsync_front_porch;

    if (h_total == 0 || v_total == 0) {
        return 0;
    }

    return timings->pclk_hz / (h_total * v_total);
}

static esp_err_t override_lcd_frame_buffers(void)
{
    dev_display_lcd_config_t *generated_cfg = NULL;
    esp_err_t ret = esp_board_manager_get_device_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&generated_cfg);
    if (ret != ESP_OK || generated_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to get LCD configuration before init: %s", esp_err_to_name(ret));
        return ret;
    }

    if (strcmp(generated_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_RGB) != 0) {
        return ESP_OK;
    }

    dev_display_lcd_config_t override_cfg = *generated_cfg;
    override_cfg.sub_cfg.rgb.panel_config.num_fbs = LCD_RGB_FRAME_BUFFER_COUNT;
    override_cfg.sub_cfg.rgb.panel_config.flags.double_fb = false;

    ret = esp_board_device_override_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD,
                                           &override_cfg,
                                           sizeof(override_cfg));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to override LCD frame buffer count: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "LCD RGB frame buffers overridden: %u -> %u",
             generated_cfg->sub_cfg.rgb.panel_config.num_fbs,
             LCD_RGB_FRAME_BUFFER_COUNT);
    return ESP_OK;
}

static esp_err_t display_hw_init(void)
{
    if (s_hw_inited) {
        return ESP_OK;
    }

    esp_err_t ret = override_lcd_frame_buffers();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = esp_board_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize board manager: %s", esp_err_to_name(ret));
        return ret;
    }

    void *lcd_handle = NULL;
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, &lcd_handle);
    if (ret != ESP_OK || !lcd_handle) {
        ESP_LOGW(TAG, "LCD device not available: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Initializing LVGL adapter...");
    const esp_lv_adapter_config_t lvgl_cfg = {
        .task_stack_size = LVGL_TASK_STACK_SIZE,
        .task_priority = LVGL_TASK_PRIORITY,
        .task_core_id = 1,
        .tick_period_ms = LVGL_TICK_PERIOD_MS,
        .task_min_delay_ms = LVGL_TASK_MIN_SLEEP_MS,
        .task_max_delay_ms = LVGL_TASK_MAX_SLEEP_MS,
        .stack_in_psram = false,
    };

    ret = esp_lv_adapter_init(&lvgl_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL adapter: %s", esp_err_to_name(ret));
        return ret;
    }

    dev_display_lcd_config_t *lcd_cfg = NULL;
    ret = esp_board_manager_get_device_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&lcd_cfg);
    if (ret != ESP_OK || lcd_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to get LCD configuration: %s", esp_err_to_name(ret));
        return ret;
    }

    dev_display_lcd_handles_t *lcd_handles = (dev_display_lcd_handles_t *)lcd_handle;
    const esp_lv_adapter_rotation_t rotation = lcd_rotation_from_config(lcd_cfg);
    esp_lv_adapter_display_config_t disp_cfg = ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(
        lcd_handles->panel_handle,
        lcd_handles->io_handle,
        lcd_cfg->lcd_width,
        lcd_cfg->lcd_height,
        rotation
    );
    disp_cfg.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;
    const uint8_t required_fbs = esp_lv_adapter_get_required_frame_buffer_count(disp_cfg.tear_avoid_mode, rotation);
    disp_cfg.profile.buffer_height = LVGL_DRAW_BUF_HEIGHT;
    disp_cfg.profile.use_psram = true;
    disp_cfg.profile.enable_ppa_accel = true;
    ESP_LOGI(TAG, "LCD tear mode=%d requires %u frame buffers, estimated refresh=%" PRIu32 " Hz",
             disp_cfg.tear_avoid_mode, required_fbs, lcd_estimated_refresh_hz(lcd_cfg));

    s_physical_disp = esp_lv_adapter_register_display(&disp_cfg);
    if (s_physical_disp == NULL) {
        ESP_LOGE(TAG, "Failed to register LCD display");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "LCD display initialized successfully");

    void *touch_device_handle = NULL;
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_LCD_TOUCH, &touch_device_handle);
    if (ret == ESP_OK && touch_device_handle != NULL) {
        dev_lcd_touch_handles_t *touch_handles = (dev_lcd_touch_handles_t *)touch_device_handle;

        if (touch_handles->touch_handle != NULL) {
            esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(s_physical_disp, touch_handles->touch_handle);
            lv_indev_t *touch_indev = esp_lv_adapter_register_touch(&touch_cfg);
            if (touch_indev != NULL) {
                lv_indev_set_scroll_limit(touch_indev, TOUCH_SCROLL_LIMIT_PX);
                ESP_LOGI(TAG, "Touch input initialized successfully");
            } else {
                ESP_LOGW(TAG, "Failed to register touch input device (continuing without touch)");
            }
        } else {
            ESP_LOGW(TAG, "Touch handle is NULL (continuing without touch)");
        }
    } else {
        ESP_LOGW(TAG, "Touch device not available: %s (continuing without touch)", esp_err_to_name(ret));
    }

    ret = esp_lv_adapter_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start LVGL adapter: %s", esp_err_to_name(ret));
        return ret;
    }

    lv_display_set_default(s_physical_disp);
    s_hw_inited = true;

    return ESP_OK;
}

static void display_set_brightness(uint8_t brightness)
{
    ESP_LOGI(TAG, "Display brightness: %u", brightness);
}

static void display_power_on(void)
{
    ESP_LOGI(TAG, "Display power on");
}

static void display_power_off(void)
{
    ESP_LOGI(TAG, "Display power off");
}

static const eos_dev_display_ops_t s_display_ops = {
    .set_brightness = display_set_brightness,
    .power_on = display_power_on,
    .power_off = display_power_off,
};

esp_err_t eos_port_display_init(void)
{
    esp_err_t ret = display_hw_init();
    if (ret != ESP_OK) {
        return ret;
    }

    if (!s_ops_registered) {
        eos_dev_display_register(&s_display_ops);
        s_ops_registered = true;
    }

    return ESP_OK;
}

lv_display_t *eos_port_display_get_physical(void)
{
    return s_physical_disp;
}
