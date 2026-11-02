#include "ds1302.h"

#include "driver/gpio.h"
#include "rom/ets_sys.h"

static gpio_num_t rtc_rst_gpio;
static gpio_num_t rtc_dat_gpio;
static gpio_num_t rtc_clk_gpio;

static void ds1302_delay(void)
{
    ets_delay_us(2);
}

static uint8_t dec_to_bcd(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

static uint8_t bcd_to_dec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static void ds1302_write_byte(uint8_t data)
{
    gpio_set_direction(rtc_dat_gpio, GPIO_MODE_OUTPUT);

    for (int i = 0; i < 8; i++)
    {
        gpio_set_level(rtc_dat_gpio, (data >> i) & 0x01);

        gpio_set_level(rtc_clk_gpio, 1);
        ds1302_delay();

        gpio_set_level(rtc_clk_gpio, 0);
        ds1302_delay();
    }
}

static uint8_t ds1302_read_byte(void)
{
    uint8_t data = 0;

    gpio_set_direction(rtc_dat_gpio, GPIO_MODE_INPUT);

    for (int i = 0; i < 8; i++)
    {
        if (gpio_get_level(rtc_dat_gpio))
        {
            data |= (1 << i);
        }

        gpio_set_level(rtc_clk_gpio, 1);
        ds1302_delay();

        gpio_set_level(rtc_clk_gpio, 0);
        ds1302_delay();
    }

    return data;
}

static void ds1302_write_register(uint8_t reg, uint8_t value)
{
    gpio_set_level(rtc_rst_gpio, 1);

    ds1302_write_byte(reg);
    ds1302_write_byte(value);

    gpio_set_level(rtc_rst_gpio, 0);
}

static uint8_t ds1302_read_register(uint8_t reg)
{
    uint8_t value;

    gpio_set_level(rtc_rst_gpio, 1);

    ds1302_write_byte(reg | 0x01);

    value = ds1302_read_byte();

    gpio_set_level(rtc_rst_gpio, 0);

    return value;
}

void ds1302_init(gpio_num_t rst_gpio,
                 gpio_num_t dat_gpio,
                 gpio_num_t clk_gpio)
{
    rtc_rst_gpio = rst_gpio;
    rtc_dat_gpio = dat_gpio;
    rtc_clk_gpio = clk_gpio;

    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask =
            (1ULL << rtc_rst_gpio) |
            (1ULL << rtc_clk_gpio)
    };

    gpio_config(&io_conf);

    gpio_set_level(rtc_rst_gpio, 0);
    gpio_set_level(rtc_clk_gpio, 0);

    ds1302_write_register(0x8E, 0x00);
}

void ds1302_set_datetime(const ds1302_datetime_t *dt)
{
    ds1302_write_register(0x8E, 0x00);

    ds1302_write_register(0x80, dec_to_bcd(dt->second) & 0x7F);
    ds1302_write_register(0x82, dec_to_bcd(dt->minute));
    ds1302_write_register(0x84, dec_to_bcd(dt->hour));

    ds1302_write_register(0x86, dec_to_bcd(dt->day));
    ds1302_write_register(0x88, dec_to_bcd(dt->month));
    ds1302_write_register(0x8A, dec_to_bcd(dt->day_of_week));
    ds1302_write_register(0x8C, dec_to_bcd(dt->year));

    ds1302_write_register(0x8E, 0x80);
}

void ds1302_get_datetime(ds1302_datetime_t *dt)
{
    dt->second =
        bcd_to_dec(ds1302_read_register(0x81) & 0x7F);

    dt->minute =
        bcd_to_dec(ds1302_read_register(0x83));

    dt->hour =
        bcd_to_dec(ds1302_read_register(0x85));

    dt->day =
        bcd_to_dec(ds1302_read_register(0x87));

    dt->month =
        bcd_to_dec(ds1302_read_register(0x89));

    dt->day_of_week =
        bcd_to_dec(ds1302_read_register(0x8B));

    dt->year =
        bcd_to_dec(ds1302_read_register(0x8D));
}