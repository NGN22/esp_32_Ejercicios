#include "lcd_i2c.h"

#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#define LCD_ADDR        0x27

/* PCF8574 */
#define LCD_RS          0x01
#define LCD_RW          0x02
#define LCD_EN          0x04
#define LCD_BL          0x08

#define LCD_D4          0x10
#define LCD_D5          0x20
#define LCD_D6          0x40
#define LCD_D7          0x80

static const char *TAG = "LCD";

static i2c_master_dev_handle_t lcd_dev = NULL;

static esp_err_t lcd_write_byte(uint8_t value)
{
    return i2c_master_transmit(lcd_dev, &value, 1, -1);
}

esp_err_t lcd_raw_write(uint8_t value)
{
    return lcd_write_byte(value);
}

static esp_err_t lcd_pulse(uint8_t data)
{
    esp_err_t ret;

    ret = lcd_write_byte(data | LCD_EN);
    if (ret != ESP_OK) {
        return ret;
    }

    esp_rom_delay_us(10);

    ret = lcd_write_byte(data & ~LCD_EN);
    if (ret != ESP_OK) {
        return ret;
    }

    esp_rom_delay_us(50);

    return ESP_OK;
}

static esp_err_t lcd_write4(uint8_t nibble, bool rs)
{
    uint8_t data = LCD_BL;

    if (rs) {
        data |= LCD_RS;
    }

    if (nibble & 0x01) data |= LCD_D4;
    if (nibble & 0x02) data |= LCD_D5;
    if (nibble & 0x04) data |= LCD_D6;
    if (nibble & 0x08) data |= LCD_D7;

    esp_err_t ret = lcd_write_byte(data);
    if (ret != ESP_OK) {
        return ret;
    }
    /*ESP_LOGI("LCD", "Nibble=%X Data=0x%02X", nibble, data);*/
    return lcd_pulse(data);
}

static esp_err_t lcd_send(uint8_t value, bool rs)
{
    esp_err_t ret;

    ret = lcd_write4((value >> 4) & 0x0F, rs);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = lcd_write4(value & 0x0F, rs);
    if (ret != ESP_OK) {
        return ret;
    }

    esp_rom_delay_us(500);

    return ESP_OK;
}

static esp_err_t lcd_command(uint8_t cmd)
{
    esp_err_t ret = lcd_send(cmd, false);
    /*Varios retardos display lento en comparacion de esp32*/
    esp_rom_delay_us(2000);

    return ret;
}

static esp_err_t lcd_data(uint8_t data)
{
    esp_err_t ret = lcd_send(data, true);

    esp_rom_delay_us(500);

    return ret;
}

esp_err_t lcd_init(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(
        bus,
        &dev_cfg,
        &lcd_dev));

    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_write4(0x03, false);
    vTaskDelay(pdMS_TO_TICKS(10));

    lcd_write4(0x03, false);
    vTaskDelay(pdMS_TO_TICKS(10));

    lcd_write4(0x03, false);
    vTaskDelay(pdMS_TO_TICKS(10));

    lcd_write4(0x02, false);
    vTaskDelay(pdMS_TO_TICKS(10));

    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_command(0x28);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_command(0x08);
    lcd_command(0x01);

    vTaskDelay(pdMS_TO_TICKS(2));

    lcd_command(0x06);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_command(0x0C);
    vTaskDelay(pdMS_TO_TICKS(5));

    ESP_LOGI(TAG, "LCD inicializado");

    return ESP_OK;
}

void lcd_clear(void)
{
    lcd_command(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_addr[] = {
        0x00,
        0x40,
        0x14,
        0x54
    };

    lcd_command(0x80 | (row_addr[row] + col));
}

void lcd_print(const char *text)
{
    while (*text) {
        lcd_data((uint8_t)*text++);
    }
}