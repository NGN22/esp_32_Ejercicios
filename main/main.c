#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

/*************************************************************
 * DISPLAY 7 SEGMENTOS
 *************************************************************/

#define SEG_A GPIO_NUM_27
#define SEG_B GPIO_NUM_14
#define SEG_C GPIO_NUM_25
#define SEG_D GPIO_NUM_32
#define SEG_E GPIO_NUM_33
#define SEG_F GPIO_NUM_12
#define SEG_G GPIO_NUM_26

/*************************************************************
 * RECEPTOR IR
 *************************************************************/

#define IR_GPIO GPIO_NUM_23
#define FRAME_SIZE 70

static const char *TAG = "DISPLAY_IR";

typedef struct
{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t f;
    uint8_t g;
} digito_t;

typedef struct
{
    uint32_t duration;
} edge_event_t;

static QueueHandle_t edge_queue;

/*************************************************************
 * TABLA DISPLAY
 *************************************************************/

static const digito_t digitos[10] =
{
    {1,1,1,1,1,1,0}, // 0
    {0,1,1,0,0,0,0}, // 1
    {1,1,0,1,1,0,1}, // 2
    {1,1,1,1,0,0,1}, // 3
    {0,1,1,0,0,1,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {1,1,1,0,0,0,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1}  // 9
};

/*************************************************************
 * VARIABLES
 *************************************************************/

static uint8_t numero_actual = 0;
static bool modo_auto = false;

/*************************************************************
 * DISPLAY
 *************************************************************/

static void configurar_display(void)
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

    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

static void mostrar_digito(uint8_t numero)
{
    if (numero > 9)
    {
        return;
    }

    gpio_set_level(SEG_A, digitos[numero].a);
    gpio_set_level(SEG_B, digitos[numero].b);
    gpio_set_level(SEG_C, digitos[numero].c);
    gpio_set_level(SEG_D, digitos[numero].d);
    gpio_set_level(SEG_E, digitos[numero].e);
    gpio_set_level(SEG_F, digitos[numero].f);
    gpio_set_level(SEG_G, digitos[numero].g);
}

/*************************************************************
 * NEC
 *************************************************************/

static bool near(uint32_t value,
                 uint32_t target,
                 uint32_t tolerance)
{
    return (value > (target - tolerance)) &&
           (value < (target + tolerance));
}

/*************************************************************
 * ACCIONES BOTONES
 *************************************************************/

static void process_command(uint8_t cmd)
{
    switch(cmd)
    {
        case 0x45:      // CH-
            if (numero_actual > 0)
                numero_actual--;

            ESP_LOGI(TAG,
                     "CH- -> %d",
                     numero_actual);
            break;

        case 0x46:      // CH
            numero_actual = 0;

            ESP_LOGI(TAG,
                     "CH -> RESET");
            break;

        case 0x47:      // CH+
            if (numero_actual < 9)
                numero_actual++;

            ESP_LOGI(TAG,
                     "CH+ -> %d",
                     numero_actual);
            break;

        case 0x43:      // PLAY
            modo_auto = !modo_auto;

            ESP_LOGI(TAG,
                     "PLAY -> AUTO %s",
                     modo_auto ? "ON" : "OFF");
            break;

        case 0x0C:      // 1
            numero_actual = 1;
            ESP_LOGI(TAG, "NUMERO 1");
            break;

        case 0x18:      // 2
            numero_actual = 2;
            ESP_LOGI(TAG, "NUMERO 2");
            break;

        case 0x5E:      // 3
            numero_actual = 3;
            ESP_LOGI(TAG, "NUMERO 3");
            break;

        default:
            return;
    }

    mostrar_digito(numero_actual);
}

/*************************************************************
 * ISR IR
 *************************************************************/

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    static int64_t last_time = 0;

    int64_t now = esp_timer_get_time();

    if (last_time != 0)
    {
        edge_event_t evt;
        evt.duration = (uint32_t)(now - last_time);

        BaseType_t hp_task_woken = pdFALSE;

        xQueueSendFromISR(
            edge_queue,
            &evt,
            &hp_task_woken);

        if (hp_task_woken)
        {
            portYIELD_FROM_ISR();
        }
    }

    last_time = now;
}

/*************************************************************
 * APP
 *************************************************************/

void app_main(void)
{
    configurar_display();

    mostrar_digito(numero_actual);

    edge_queue = xQueueCreate(
        128,
        sizeof(edge_event_t));

    gpio_config_t ir_conf = {
        .pin_bit_mask = (1ULL << IR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };

    ESP_ERROR_CHECK(gpio_config(&ir_conf));

    ESP_ERROR_CHECK(
        gpio_install_isr_service(
            ESP_INTR_FLAG_IRAM));

    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            IR_GPIO,
            gpio_isr_handler,
            NULL));

    ESP_LOGI(TAG,
             "Display + IR iniciado");

    edge_event_t evt;

    uint32_t frame[FRAME_SIZE];
    uint8_t index = 0;

    TickType_t ultimo_auto = xTaskGetTickCount();

    while (1)
    {
        if (xQueueReceive(
                edge_queue,
                &evt,
                pdMS_TO_TICKS(10)))
        {
            if (evt.duration > 8500 &&
                evt.duration < 10000)
            {
                index = 0;
            }

            if (index < FRAME_SIZE)
            {
                frame[index++] = evt.duration;
            }

            if (index >= 66)
            {
                if (near(frame[0], 9000, 1500) &&
                    near(frame[1], 4500, 1000))
                {
                    uint32_t code = 0;

                    for (int bit = 0; bit < 32; bit++)
                    {
                        uint32_t high_time =
                            frame[3 + bit * 2];

                        if (high_time > 1200)
                        {
                            code |= (1UL << bit);
                        }
                    }

                    uint8_t cmd =
                        (code >> 16) & 0xFF;

                    process_command(cmd);
                }

                index = 0;
            }
        }

        if (modo_auto)
        {
            TickType_t ahora = xTaskGetTickCount();

            if ((ahora - ultimo_auto) >
                pdMS_TO_TICKS(1000))
            {
                ultimo_auto = ahora;

                numero_actual++;

                if (numero_actual > 9)
                {
                    numero_actual = 0;
                }

                mostrar_digito(numero_actual);

                ESP_LOGI(TAG,
                         "AUTO -> %d",
                         numero_actual);
            }
        }
    }
}