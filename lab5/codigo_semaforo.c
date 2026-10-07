#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <stdio.h>

#define CICLOS_MS 10000

typedef struct {
    const char *name;
    gpio_num_t pino;
    uint32_t offset_ms;
    uint32_t duracao_ms;
} led_cfg_t;

static void tarefa_led(void *arg)
{
    const led_cfg_t *cfg = arg;

    gpio_reset_pin(cfg->pino);
    gpio_set_direction(cfg->pino, GPIO_MODE_OUTPUT);
    gpio_set_level(cfg->pino, 0);

    if (cfg->offset_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(cfg->offset_ms));
    }

    TickType_t proximo = xTaskGetTickCount();
    while (1) {
        gpio_set_level(cfg->pino, 1);
        printf("%s: LIGA\n", cfg->name);
        vTaskDelayUntil(&proximo, pdMS_TO_TICKS(cfg->duracao_ms));

        gpio_set_level(cfg->pino, 0);
        printf("%s: DESLIGA\n", cfg->name);
        vTaskDelayUntil(&proximo, pdMS_TO_TICKS(CICLOS_MS - cfg->duracao_ms));
    }
}

static const led_cfg_t cfg_verde = {
    .name = "VERDE",
    .pino = GPIO_NUM_2,
    .offset_ms = 0,
    .duracao_ms = 5000
};

static const led_cfg_t cfg_amarelo = {
    .name = "AMARELO",
    .pino = GPIO_NUM_4,
    .offset_ms = 5000,
    .duracao_ms = 1000
};

static const led_cfg_t cfg_vermelho = {
    .name = "VERMELHO",
    .pino = GPIO_NUM_5,
    .offset_ms = 6000,
    .duracao_ms = 4000
};

void app_main(void)
{
    xTaskCreate(tarefa_led, "tarefa_led_verde", 2048, (void *)&cfg_verde, 3, NULL);
    xTaskCreate(tarefa_led, "tarefa_led_amarelo", 2048, (void *)&cfg_amarelo, 3, NULL);
    xTaskCreate(tarefa_led, "tarefa_led_vermelho", 2048, (void *)&cfg_vermelho, 3, NULL);
}