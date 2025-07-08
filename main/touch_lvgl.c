#include "ft6x36.h"
#include "touch_lvgl.h"
#include "lvgl.h"
#include "esp_log.h"

#define TAG "FT6X36"

extern void toggle_background_color(void);  // main.c'deki fonksiyon

static bool was_touched = false;

static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t x, y;
    bool touched = ft6x36_get_touch(&x, &y);
    
    if (touched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
        
        if (!was_touched) {
            was_touched = true;
            ESP_LOGI(TAG, "Dokunma algılandı! X=%d Y=%d", x, y);
            toggle_background_color();
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
        was_touched = false;
    }
}


void touch_lvgl_init(void) {
    ft6x36_init();

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    ESP_LOGI(TAG, "LVGL input device kaydı tamam.");
}
