#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_lcd_st7701.h"
#include "esp_log.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>

#include "touch_lvgl.h"

const char *TAG = "main";

#define LCD_PIXEL_CLOCK_HZ     (16 * 1000 * 1000)
#define LCD_H_RES              480
#define LCD_V_RES              480
#define LCD_NUM_FB             2

#define PIN_NUM_HSYNC          21
#define PIN_NUM_VSYNC          47
#define PIN_NUM_DE             48
#define PIN_NUM_PCLK           45

#define PIN_NUM_DATA0          14
#define PIN_NUM_DATA1          13
#define PIN_NUM_DATA2          12
#define PIN_NUM_DATA3          11
#define PIN_NUM_DATA4          10
#define PIN_NUM_DATA5          9
#define PIN_NUM_DATA6          46
#define PIN_NUM_DATA7          3
#define PIN_NUM_DATA8          20
#define PIN_NUM_DATA9          19
#define PIN_NUM_DATA10         8
#define PIN_NUM_DATA11         18
#define PIN_NUM_DATA12         17
#define PIN_NUM_DATA13         16
#define PIN_NUM_DATA14         15
#define PIN_NUM_DATA15         7

#define PIN_NUM_DISP_EN        -1

#define LCD_MISO               40
#define LCD_SCLK               39
#define LCD_CS                 38
#define LCD_RESET              42

static const st7701_lcd_init_cmd_t lcd_init_cmds[] = {
    {0x01, (uint8_t []){0x00}, 0, 6},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t []){0x3D, 0x00}, 2, 0},
    {0xC1, (uint8_t []){0x08, 0x04}, 2, 0},
    {0xC3, (uint8_t []){0x00, 0x10, 0x08}, 3, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t []){0x08}, 1, 0}, 
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC2, (uint8_t []){0x07, 0x0A}, 2, 0},
    {0xCC, (uint8_t []){0x30}, 1, 0},
    {0xB0, (uint8_t []){0x40, 0x07, 0x53, 0x0E, 0x12, 0x07, 0x0A, 0x09, 0x09, 0x26, 0x05, 0x10, 0x0D, 0x6E, 0x3B, 0xD6}, 16, 0},
    {0xB1, (uint8_t []){0x40, 0x17, 0x5C, 0x0D, 0x11, 0x06, 0x08, 0x08, 0x08, 0x03, 0x12, 0x11, 0x65, 0x28, 0xE8}, 15, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t []){0x4D}, 1, 0},
    {0xB1, (uint8_t []){0x4C}, 1, 0},
    {0xB2, (uint8_t []){0x81}, 1, 0},
    {0xB3, (uint8_t []){0x80}, 1, 0},
    {0xB5, (uint8_t []){0x4C}, 1, 0},
    {0xB7, (uint8_t []){0x85}, 1, 0},
    {0xB8, (uint8_t []){0x33}, 1, 0},
    {0xC1, (uint8_t []){0x78}, 1, 0},
    {0xC2, (uint8_t []){0x78}, 1, 0},
    {0xD0, (uint8_t []){0x88}, 1, 100},
    {0xE0, (uint8_t []){0x00, 0x00, 0x02}, 3, 0},
    {0xE1, (uint8_t []){0x05, 0x30, 0x00, 0x00, 0x06, 0x30, 0x00, 0x00, 0x0E, 0x30, 0x30}, 11, 0},
    {0xE2, (uint8_t []){0x10, 0x10, 0x30, 0x30, 0xF4, 0x00, 0x00, 0x00, 0xF4, 0x00, 0x00, 0x00}, 12, 0},
    {0xE3, (uint8_t []){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t []){0x0A, 0xF4, 0x30, 0xF0, 0x0C, 0xF6, 0x30, 0xF0, 0x06, 0xF0, 0x30, 0xF0, 0x08, 0xF2, 0x30, 0xF0}, 16, 0},
    {0xE6, (uint8_t []){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t []){0x0B, 0xF5, 0x30, 0xF0, 0x0D, 0xF7, 0x30, 0xF0, 0x07, 0xF1, 0x30, 0xF0, 0x09, 0xF3, 0x30, 0xF0}, 16, 0},
    {0xE9, (uint8_t []){0x36, 0x01}, 2, 0}, 
    {0xEB, (uint8_t []){0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x33}, 7, 0},
    {0xED, (uint8_t []){0x20, 0xFA, 0xB7, 0x76, 0x65, 0x54, 0x4F, 0xFF, 0xFF, 0xF4, 0x45, 0x56, 0x67, 0x7B, 0xAF, 0x02}, 16, 0},
    {0xEF, (uint8_t []){0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}, 6, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x3A, (uint8_t []){0x55}, 1, 0},
    {0x29, (uint8_t []){0x00}, 0, 100},
};

esp_lcd_panel_handle_t init_lcd(void) {
    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO,
        .cs_gpio_num = LCD_CS,
        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = LCD_SCLK,
        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = LCD_MISO,
        .io_expander = NULL,
    };

    esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

    esp_lcd_rgb_timing_t timing = {
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .h_res = LCD_H_RES,
        .v_res = LCD_V_RES,
        .hsync_back_porch = 40,
        .hsync_front_porch = 20,
        .hsync_pulse_width = 1,
        .vsync_back_porch = 8,
        .vsync_front_porch = 4,
        .vsync_pulse_width = 1,
        .flags.pclk_active_neg = false,
    };

    esp_lcd_rgb_panel_config_t rgb_config = {
        .data_width = 16,
        .psram_trans_align = 64,
        .num_fbs = LCD_NUM_FB,
        .bounce_buffer_size_px = LCD_H_RES * 10,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = PIN_NUM_DISP_EN,
        .pclk_gpio_num = PIN_NUM_PCLK,
        .vsync_gpio_num = PIN_NUM_VSYNC,
        .hsync_gpio_num = PIN_NUM_HSYNC,
        .de_gpio_num = PIN_NUM_DE,
        .timings = timing,
        .flags.fb_in_psram = true,
    };

    int data_pins[16] = {
        PIN_NUM_DATA0, PIN_NUM_DATA1, PIN_NUM_DATA2, PIN_NUM_DATA3,
        PIN_NUM_DATA4, PIN_NUM_DATA5, PIN_NUM_DATA6, PIN_NUM_DATA7,
        PIN_NUM_DATA8, PIN_NUM_DATA9, PIN_NUM_DATA10, PIN_NUM_DATA11,
        PIN_NUM_DATA12, PIN_NUM_DATA13, PIN_NUM_DATA14, PIN_NUM_DATA15
    };
    memcpy(rgb_config.data_gpio_nums, data_pins, sizeof(data_pins));

    st7701_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st7701_lcd_init_cmd_t),
        .flags = {
            .mirror_by_cmd = 1,
            .use_mipi_interface = 0,
            .auto_del_panel_io = 0,
        }
    };

    esp_lcd_panel_dev_config_t dev_config = {
        .reset_gpio_num = LCD_RESET,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };

    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &dev_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    return panel_handle;
}

// flush_cb fonksiyonu C uyumlu şekilde dışarı alındı
void my_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t handle = (esp_lcd_panel_handle_t)drv->user_data;
    esp_lcd_panel_draw_bitmap(handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

void lv_port_display_init(esp_lcd_panel_handle_t panel_handle) {
    lv_init();

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[LCD_H_RES * 100];
    static lv_color_t buf2[LCD_H_RES * 100];
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_H_RES * 100);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = my_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_drv_register(&disp_drv);
}

// Başlangıçta mavi, tıklanınca kırmızı olur
static bool is_red = false;

void toggle_background_color() {
    is_red = !is_red;

    lv_color_t new_color = is_red ? lv_color_hex(0xFF0000) : lv_color_hex(0x003366);
    lv_obj_set_style_bg_color(lv_scr_act(), new_color, LV_PART_MAIN);
    lv_obj_invalidate(lv_scr_act()); // Ekranı yeniden çiz
    ESP_LOGI("TOUCH", "Arka plan rengi değişti: %s", is_red ? "KIRMIZI" : "MAVI");
}

// Event callback: Sadece label'e basıldığında tetiklenir
static void label_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        is_red = !is_red;
        uint32_t color = is_red ? 0xFF0000 : 0x0000FF;
        ESP_LOGI(TAG, "Arka plan rengi değiştiriliyor: %s", is_red ? "KIRMIZI" : "MAVI");
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(color), 0);
    }
}

static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(5);
    lv_timer_handler();
}

void app_main(void)
{
    esp_lcd_panel_handle_t panel = init_lcd();
    lv_port_display_init(panel);
    touch_lvgl_init();

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x003366), 0); // Başlangıç rengi: koyu mavi

    // Orta ekrana büyük tıklanabilir label yerleştir
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Yusuf AYSU");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_color(label, lv_color_hex(0xAAAAAA), 0); // Arka planı görünür yap
    lv_obj_set_style_bg_opa(label, LV_OPA_50, 0);
    lv_obj_set_size(label, 300, 100);  // Tıklanabilir alan genişliği
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label, label_event_cb, LV_EVENT_CLICKED, NULL);

    const esp_timer_create_args_t lvgl_timer_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_timer;
    esp_timer_create(&lvgl_timer_args, &lvgl_timer);
    esp_timer_start_periodic(lvgl_timer, 5 * 1000); // 5 ms

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*
// LVGL zamanlayıcı callback'i
void lvgl_tick_cb(void *arg) {
    lv_tick_inc(5);        // Zamanı güncelle
    lv_timer_handler();    // LVGL işle
}

// Etiket tıklama event callback'i
void label_event_cb(lv_event_t *e) {
    ESP_LOGI("TOUCH", "Etiket tıklandı");
    lv_obj_t *scr = lv_scr_act();

    if (is_red) {
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x003366), 0); // mavi
    } else {
        lv_obj_set_style_bg_color(scr, lv_color_hex(0xFF0000), 0); // kırmızı
    }

    is_red = !is_red;
}

#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA 5   // Senin SDA pini
#define I2C_MASTER_SCL 4   // Senin SCL pini
#define I2C_MASTER_FREQ_HZ 100000

void scan_i2c_bus() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    ESP_LOGI("I2C", "I2C taraması başlatılıyor...");
    for (int addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI("I2C", "Cihaz bulundu: 0x%02X", addr);
        }
    }
}

void app_main(void) {
    esp_lcd_panel_handle_t panel = init_lcd();
    lv_port_display_init(panel);     // LCD ve LVGL başlat
    touch_lvgl_init();              // Dokunmatik LVGL’ye bağlan

    // Etiket + arka plan
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x003366), 0);

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Yusuf AYSU");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label, label_event_cb, LV_EVENT_CLICKED, NULL);

    // LVGL zamanlayıcı
    const esp_timer_create_args_t lvgl_timer_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_timer;
    esp_timer_create(&lvgl_timer_args, &lvgl_timer);
    esp_timer_start_periodic(lvgl_timer, 5 * 1000);

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
*/