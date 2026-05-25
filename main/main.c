#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "esp_lv_adapter.h"

#include "lvgl.h"
#include "eos_config.h"
#include "eos_core.h"
#include "eos_crown.h"
#include "eos_esp_port.h"
#include "eos_side_button.h"

lv_display_t *eos_virtual_display_create(lv_obj_t *parent, lv_coord_t hor_res, lv_coord_t ver_res);

static const char *TAG = "main";

#define EOS_TASK_STACK_SIZE    (16 * 1024)
#define EOS_TASK_PRIORITY      4
#define EOS_TASK_DELAY_MS      5
#define WATCH_FRAME_WIDTH      10
#define WATCH_FRAME_OUTLINE    4
#define WATCH_CROWN_WIDTH      24
#define WATCH_CROWN_HEIGHT     66
#define WATCH_SIDE_WIDTH       12
#define WATCH_SIDE_HEIGHT      98

static lv_display_t *s_physical_disp;
static lv_display_t *s_eos_disp;
lv_obj_t *brightness_mask = NULL;

static void crown_clicked_cb(lv_event_t *e)
{
    (void)e;
    eos_crown_button_report(EOS_BUTTON_STATE_CLICKED);
}

static void side_button_clicked_cb(lv_event_t *e)
{
    (void)e;
    eos_side_button_report(EOS_BUTTON_STATE_CLICKED);
}

static void add_button_press_style(lv_obj_t *obj)
{
    static lv_style_t style_pressed;
    static bool style_inited;

    if (!style_inited) {
        lv_style_init(&style_pressed);
        lv_style_set_bg_color(&style_pressed, lv_color_hex(0x4a4a4a));
        lv_style_set_bg_opa(&style_pressed, LV_OPA_COVER);
        style_inited = true;
    }
    lv_obj_add_style(obj, &style_pressed, LV_STATE_PRESSED);
}

static void lock_shell_obj(lv_obj_t *obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_MOMENTUM);
}

static lv_obj_t *create_watch_button(lv_obj_t *parent, int32_t x, int32_t y,
                                     int32_t w, int32_t h, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_remove_style_all(btn);
    lock_shell_obj(btn);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1b1b1b), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x3c3c3c), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_shadow_width(btn, 6, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
    lv_obj_set_style_shadow_color(btn, lv_color_black(), 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    add_button_press_style(btn);
    return btn;
}

static void create_side_buttons_overlay(uint16_t physical_w, uint16_t physical_h)
{
    lv_display_t *prev_disp = lv_display_get_default();
    lv_display_set_default(s_physical_disp);

    lv_obj_t *top = lv_layer_top();
    const int32_t watch_frame_w = EOS_DISPLAY_WIDTH + WATCH_FRAME_WIDTH * 2;
    const int32_t frame_right = ((int32_t)physical_w + watch_frame_w) / 2;
    const int32_t button_x = frame_right;
    const int32_t crown_y = physical_h > WATCH_CROWN_HEIGHT ? (physical_h - WATCH_CROWN_HEIGHT) / 2 - 60 : 0;
    const int32_t side_x = frame_right;
    const int32_t side_y = physical_h > WATCH_SIDE_HEIGHT ? (physical_h - WATCH_SIDE_HEIGHT) / 2 + 80 : 0;

    create_watch_button(top, button_x, LV_MAX(crown_y, 0),
                        WATCH_CROWN_WIDTH, WATCH_CROWN_HEIGHT, crown_clicked_cb);
    create_watch_button(top, side_x, LV_MAX(side_y, 0), WATCH_SIDE_WIDTH, WATCH_SIDE_HEIGHT,
                        side_button_clicked_cb);

    ESP_LOGI(TAG, "Side buttons created near watch frame: physical=%ux%u frame_right=%" PRId32,
             physical_w, physical_h, frame_right);
    if (prev_disp) {
        lv_display_set_default(prev_disp);
    }
}

static lv_display_t *create_virtual_desktop(void)
{
    lv_display_set_default(s_physical_disp);

    lv_group_set_default(lv_group_create());
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *desktop = lv_obj_create(screen);
    lv_obj_remove_style_all(desktop);
    lv_obj_set_size(desktop,
                    lv_display_get_horizontal_resolution(s_physical_disp),
                    lv_display_get_vertical_resolution(s_physical_disp));
    lv_obj_remove_flag(desktop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(desktop);

    const int32_t watch_frame_width = EOS_DISPLAY_WIDTH + WATCH_FRAME_WIDTH * 2;
    const int32_t watch_frame_height = EOS_DISPLAY_HEIGHT + WATCH_FRAME_WIDTH * 2;

    lv_obj_t *watch_frame = lv_obj_create(desktop);
    lv_obj_remove_style_all(watch_frame);
    lv_obj_set_size(watch_frame, watch_frame_width, watch_frame_height);
    lv_obj_center(watch_frame);
    lv_obj_set_style_bg_color(watch_frame, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(watch_frame, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(watch_frame, EOS_DISPLAY_RADIUS + WATCH_FRAME_WIDTH, 0);
    lv_obj_set_style_outline_color(watch_frame, lv_color_hex(0x1f1f1f), 0);
    lv_obj_set_style_outline_opa(watch_frame, LV_OPA_COVER, 0);
    lv_obj_set_style_outline_width(watch_frame, WATCH_FRAME_OUTLINE, 0);
    lv_obj_set_style_outline_pad(watch_frame, -1, 0);

    lv_obj_t *vd_container = lv_obj_create(desktop);
    lv_obj_remove_style_all(vd_container);
    lv_obj_set_size(vd_container, EOS_DISPLAY_WIDTH, EOS_DISPLAY_HEIGHT);
    lv_obj_center(vd_container);
    lv_obj_remove_flag(vd_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(vd_container, EOS_DISPLAY_RADIUS, 0);
    lv_obj_set_style_clip_corner(vd_container, true, 0);

    lv_display_t *disp = eos_virtual_display_create(vd_container, EOS_DISPLAY_WIDTH, EOS_DISPLAY_HEIGHT);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to create virtual ElenixOS display");
        return NULL;
    }
    lv_display_set_default(disp);

#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
    lv_sysmon_hide_performance(disp);
#endif
#if LV_USE_SYSMON && LV_USE_MEM_MONITOR
    lv_sysmon_hide_memory(disp);
#endif

    brightness_mask = lv_obj_create(desktop);
    lv_obj_set_size(brightness_mask, EOS_DISPLAY_WIDTH, EOS_DISPLAY_HEIGHT);
    lv_obj_set_style_bg_color(brightness_mask, lv_color_black(), 0);
    lv_obj_set_style_border_width(brightness_mask, 0, 0);
    lv_obj_set_style_opa(brightness_mask, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(brightness_mask, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(brightness_mask, EOS_DISPLAY_RADIUS, 0);
    lv_obj_center(brightness_mask);

    ESP_LOGI(TAG, "Virtual ElenixOS display created: %" PRId32 "x%" PRId32,
             (int32_t)EOS_DISPLAY_WIDTH, (int32_t)EOS_DISPLAY_HEIGHT);
    return disp;
}

static esp_err_t elenixos_init(void)
{
    esp_err_t ret = eos_esp_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ElenixOS port initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_physical_disp = eos_port_display_get_physical();
    if (!s_physical_disp) {
        ESP_LOGE(TAG, "Display port did not provide a physical display");
        return ESP_FAIL;
    }

    if (esp_lv_adapter_lock(-1) != ESP_OK) {
        ESP_LOGE(TAG, "LVGL lock failed");
        return ESP_FAIL;
    }

    s_eos_disp = create_virtual_desktop();
    if (!s_eos_disp) {
        return ESP_FAIL;
    }
    lv_display_set_default(s_eos_disp);
    eos_init();
    create_side_buttons_overlay(lv_display_get_physical_horizontal_resolution(s_physical_disp),
                                lv_display_get_physical_vertical_resolution(s_physical_disp));
    lv_display_set_default(s_eos_disp);

    esp_lv_adapter_unlock();

    return ESP_OK;
}

static void elenixos_task(void *arg)
{
    (void)arg;

    esp_err_t ret = elenixos_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ElenixOS initialization failed");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        if (esp_lv_adapter_lock(-1) == ESP_OK) {
            eos_esp_port_tick();
            esp_lv_adapter_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(EOS_TASK_DELAY_MS));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting application");

    BaseType_t task_ret = xTaskCreatePinnedToCore(
        elenixos_task,
        "elenixos",
        EOS_TASK_STACK_SIZE,
        NULL,
        EOS_TASK_PRIORITY,
        NULL,
        1
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ElenixOS task");
        return;
    }
}
