#ifndef DS1302_H
#define DS1302_H

#include <stdint.h>
#include "driver/gpio.h"

typedef struct
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;

    uint8_t day;
    uint8_t month;
    uint8_t year;

    uint8_t day_of_week;

} ds1302_datetime_t;

void ds1302_init(gpio_num_t rst_gpio,
                 gpio_num_t dat_gpio,
                 gpio_num_t clk_gpio);

void ds1302_set_datetime(const ds1302_datetime_t *dt);

void ds1302_get_datetime(ds1302_datetime_t *dt);

#endif
