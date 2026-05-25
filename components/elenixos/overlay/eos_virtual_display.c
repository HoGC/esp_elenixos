/**
 * @file eos_virtual_display.c
 * @brief ESP port virtual display
 */

#include "eos_config.h"

#if EOS_USE_VIRTUAL_DISPLAY

#include <stdbool.h>
#include <stdint.h>

#define EOS_LOG_TAG "VirtualDisplayPort"
#include "eos_log.h"
#include "eos_mem.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t *canvas;
    lv_color_t *canvas_buf;
    lv_coord_t hor_res;
    lv_coord_t ver_res;
    lv_color_format_t cf;
    lv_display_t *disp;
    lv_indev_t *indev;
    int32_t touch_x;
    int32_t touch_y;
    bool pressed;
} eos_virtual_display_t;

static void virtual_display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)px_map;

    eos_virtual_display_t *vd = lv_display_get_driver_data(disp);
    if (vd && vd->canvas) {
        lv_area_t canvas_area;
        lv_obj_get_coords(vd->canvas, &canvas_area);

        lv_area_t inv_area = {
            .x1 = canvas_area.x1 + area->x1,
            .y1 = canvas_area.y1 + area->y1,
            .x2 = canvas_area.x1 + area->x2,
            .y2 = canvas_area.y1 + area->y2,
        };
        lv_obj_invalidate_area(vd->canvas, &inv_area);
    }

    lv_display_flush_ready(disp);
}

static void canvas_event_cb(lv_event_t *e)
{
    eos_virtual_display_t *vd = (eos_virtual_display_t *)lv_event_get_user_data(e);
    if (!vd) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED) {
        vd->pressed = false;
        return;
    }

    lv_indev_t *act_indev = lv_indev_get_act();
    if (!act_indev) {
        return;
    }

    lv_point_t pt = {0, 0};
    lv_indev_get_point(act_indev, &pt);

    lv_area_t canvas_area;
    lv_obj_get_coords(vd->canvas, &canvas_area);

    int32_t x = pt.x - canvas_area.x1;
    int32_t y = pt.y - canvas_area.y1;

    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x >= vd->hor_res) {
        x = vd->hor_res - 1;
    }
    if (y >= vd->ver_res) {
        y = vd->ver_res - 1;
    }

    vd->touch_x = x;
    vd->touch_y = y;
    vd->pressed = code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING;
}

static void virtual_input_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    eos_virtual_display_t *vd = (eos_virtual_display_t *)lv_indev_get_user_data(indev);
    if (!vd) {
        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = 0;
        data->point.y = 0;
        return;
    }

    data->point.x = vd->touch_x;
    data->point.y = vd->touch_y;
    data->state = vd->pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

lv_display_t *eos_virtual_display_create(lv_obj_t *parent, lv_coord_t hor_res, lv_coord_t ver_res)
{
    eos_virtual_display_t *vd = eos_malloc_zeroed(sizeof(eos_virtual_display_t));
    if (!vd) {
        return NULL;
    }

    vd->hor_res = hor_res;
    vd->ver_res = ver_res;
    vd->cf = LV_COLOR_FORMAT_NATIVE;

    const uint32_t stride = lv_draw_buf_width_to_stride(hor_res, vd->cf);
    const size_t buf_size = stride * ver_res;

    vd->canvas_buf = eos_malloc_zeroed(buf_size);
    if (!vd->canvas_buf) {
        eos_free(vd);
        return NULL;
    }

    vd->canvas = lv_canvas_create(parent);
    if (!vd->canvas) {
        eos_free(vd->canvas_buf);
        eos_free(vd);
        return NULL;
    }

    lv_canvas_set_buffer(vd->canvas, vd->canvas_buf, hor_res, ver_res, vd->cf);
    lv_obj_center(vd->canvas);
    lv_obj_add_flag(vd->canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(vd->canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(vd->canvas, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_event_cb(vd->canvas, canvas_event_cb, LV_EVENT_PRESSED, vd);
    lv_obj_add_event_cb(vd->canvas, canvas_event_cb, LV_EVENT_PRESSING, vd);
    lv_obj_add_event_cb(vd->canvas, canvas_event_cb, LV_EVENT_RELEASED, vd);

    vd->disp = lv_display_create(hor_res, ver_res);
    if (!vd->disp) {
        eos_free(vd->canvas_buf);
        eos_free(vd);
        return NULL;
    }

    lv_display_set_buffers(vd->disp, vd->canvas_buf, NULL, buf_size, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_driver_data(vd->disp, vd);
    lv_display_set_flush_cb(vd->disp, virtual_display_flush_cb);

    vd->indev = lv_indev_create();
    if (!vd->indev) {
        EOS_LOG_E("failed to create indev");
        lv_display_delete(vd->disp);
        eos_free(vd->canvas_buf);
        eos_free(vd);
        return NULL;
    }

    lv_indev_set_type(vd->indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(vd->indev, virtual_input_read);
    lv_indev_set_user_data(vd->indev, vd);
    lv_indev_set_display(vd->indev, vd->disp);

    return vd->disp;
}

#endif /* EOS_USE_VIRTUAL_DISPLAY */
