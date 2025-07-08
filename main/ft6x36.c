#include "ft6x36.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define TAG "FT6X36"
#define FT6X36_ADDR 0x38
#define REG_TD_STATUS 0x02
#define REG_TOUCH1_XH 0x03

static i2c_port_t i2c_port = I2C_NUM_0;

void ft6x36_init()
{
    ESP_LOGI("FT6X36", "I2C başlatılıyor (SDA=%d, SCL=%d)...", SDA, SCL);
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA,
        .scl_io_num = SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(i2c_port, &conf);
    i2c_driver_install(i2c_port, conf.mode, 0, 0, 0);
    ESP_LOGI("FT6X36", "I2C başlatıldı!");
}

static esp_err_t ft6x36_read_bytes(uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (FT6X36_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (FT6X36_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

bool ft6x36_get_touch(uint16_t *x, uint16_t *y)
{
    uint8_t data[4];
    esp_err_t ret = ft6x36_read_bytes(REG_TD_STATUS, data, 1);
    if (ret != ESP_OK) {
        ESP_LOGW("FT6X36", "TD_STATUS okunamadı! ret = 0x%x", ret);
        return false;
    }

    if (data[0] == 0) {
        return false;
    }

    if (ft6x36_read_bytes(REG_TOUCH1_XH, data, 4) != ESP_OK) {
        ESP_LOGW("FT6X36", "TOUCH1 verisi okunamadı");
        return false;
    }

    *x = ((data[0] & 0x0F) << 8) | data[1];
    *y = ((data[2] & 0x0F) << 8) | data[3];

    ESP_LOGI("FT6X36", "Dokunma algılandı! X=%d Y=%d", *x, *y);

    return true;
}