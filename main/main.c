#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "esp_adc/adc_oneshot.h"

#include "driver/ledc.h"

#include "driver/gpio.h"

#define LED_GPIO        GPIO_NUM_18

#define ADC_CHANNEL     ADC_CHANNEL_6      // GPIO34
#define PWM_FREQUENCY   5000
#define PWM_RESOLUTION  LEDC_TIMER_13_BIT

static const char *TAG = "ADC_PWM";

void app_main(void)
{
    /* ==========================
       ADC
       ========================== */

    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t adc_init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &adc_init_cfg,
            &adc_handle
        )
    );

    adc_oneshot_chan_cfg_t adc_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            ADC_CHANNEL,
            &adc_cfg
        )
    );

    /* ==========================
       PWM LEDC
       ========================== */

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ESP_ERROR_CHECK(
        ledc_timer_config(&ledc_timer)
    );

    ledc_channel_config_t ledc_channel = {
        .gpio_num = LED_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ESP_ERROR_CHECK(
        ledc_channel_config(&ledc_channel)
    );

    ESP_LOGI(TAG, "Control de brillo iniciado");

    while (1)
    {
        int adc_raw = 0;

        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc_handle,
                ADC_CHANNEL,
                &adc_raw
            )
        );

        uint32_t duty =
            ((uint32_t)adc_raw * 8191) / 4095;

        ESP_ERROR_CHECK(
            ledc_set_duty(
                LEDC_LOW_SPEED_MODE,
                LEDC_CHANNEL_0,
                duty
            )
        );

        ESP_ERROR_CHECK(
            ledc_update_duty(
                LEDC_LOW_SPEED_MODE,
                LEDC_CHANNEL_0
            )
        );

        ESP_LOGI(TAG,
                 "ADC=%d PWM=%lu",
                 adc_raw,
                 (unsigned long)duty);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}