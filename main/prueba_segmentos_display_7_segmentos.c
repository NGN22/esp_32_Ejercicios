#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

#define SEG_A GPIO_NUM_27
#define SEG_B GPIO_NUM_14
#define SEG_C GPIO_NUM_25
#define SEG_D GPIO_NUM_32
#define SEG_E GPIO_NUM_33
#define SEG_F GPIO_NUM_12
#define SEG_G GPIO_NUM_26

static const char *TAG = "DISPLAY";

static void apagar_todos(void)
{
    gpio_set_level(SEG_A, 0);
    gpio_set_level(SEG_B, 0);
    gpio_set_level(SEG_C, 0);
    gpio_set_level(SEG_D, 0);
    gpio_set_level(SEG_E, 0);
    gpio_set_level(SEG_F, 0);
    gpio_set_level(SEG_G, 0);
}

static void configurar_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask =
            (1ULL << SEG_A) |
            (1ULL << SEG_B) |
            (1ULL << SEG_C) |
            (1ULL << SEG_D) |
            (1ULL << SEG_E) |
            (1ULL << SEG_F) |
            (1ULL << SEG_G),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_conf);

    apagar_todos();
}

void app_main(void)
{
    configurar_gpio();

    while (1)
    {
        ESP_LOGI(TAG, "Segmento A -> GPIO27");
        apagar_todos();
        gpio_set_level(SEG_A, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Segmento B -> GPIO14");
        apagar_todos();
        gpio_set_level(SEG_B, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Segmento C -> GPIO25");
        apagar_todos();
        gpio_set_level(SEG_C, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Segmento D -> GPIO32");
        apagar_todos();
        gpio_set_level(SEG_D, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Segmento E -> GPIO33");
        apagar_todos();
        gpio_set_level(SEG_E, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Segmento F -> GPIO12");
        apagar_todos();
        gpio_set_level(SEG_F, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Segmento G -> GPIO26");
        apagar_todos();
        gpio_set_level(SEG_G, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}