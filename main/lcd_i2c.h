#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "esp_err.h"
#include "driver/i2c_master.h"

esp_err_t lcd_init(i2c_master_bus_handle_t bus);
void lcd_clear(void);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_print(const char *text);
esp_err_t lcd_raw_write(uint8_t value);

#endif