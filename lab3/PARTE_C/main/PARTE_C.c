// Semana 3 — botão com pull-up interno + captura de bouncing via interrupção (ANYEDGE)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <stdio.h>

#define LED   GPIO_NUM_2
#define BTN   GPIO_NUM_0
#define DEBOUNCE_MS 50   // ainda existe, mas agora filtrado dentro da ISR

static QueueHandle_t fila_bordas;

// ISR: roda em contexto de interrupção — deve ser curta, sem printf,
// sem alocação, sem chamadas bloqueantes.
static void IRAM_ATTR isr_botao(void *arg)
{
    int nivel = gpio_get_level(BTN);
    int64_t agora_us = esp_timer_get_time();

    // Empacota nível + timestamp e manda para a fila.
    // xQueueSendFromISR é a versão segura para uso dentro de ISR.
    struct { int nivel; int64_t t_us; } msg = { nivel, agora_us };
    BaseType_t acordou_task_maior_prioridade = pdFALSE;
    xQueueSendFromISR(fila_bordas, &msg, &acordou_task_maior_prioridade);
    if (acordou_task_maior_prioridade) {
        portYIELD_FROM_ISR();
    }
}

void app_main(void)
{
    gpio_reset_pin(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    gpio_reset_pin(BTN);
    gpio_set_direction(BTN, GPIO_MODE_INPUT);
    gpio_pullup_en(BTN);
    gpio_set_intr_type(BTN, GPIO_INTR_ANYEDGE);   // dispara em subida E descida

    fila_bordas = xQueueCreate(64, sizeof(struct { int nivel; int64_t t_us; }));

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN, isr_botao, NULL);

    int led = 0, bordas_totais = 0;
    int64_t t_ok_us = 0;   // instante (us) a partir do qual aceitamos novo evento válido

    struct { int nivel; int64_t t_us; } msg;

    while (1) {
        // Bloqueia aqui até a ISR enfileirar uma borda — sem polling.
        if (xQueueReceive(fila_bordas, &msg, portMAX_DELAY)) {
            bordas_totais++;   // TODA borda física, incluindo bounce

            // Só conta "evento" (clique válido) na descida (1->0) e fora
            // da janela de debounce — igual à lógica original, mas agora
            // aplicada sobre bordas reais, não sobre amostras periódicas.
            if (msg.nivel == 0 && msg.t_us >= t_ok_us) {
                led = !led;
                gpio_set_level(LED, led);
                printf("borda total #%d\n", bordas_totais);
                t_ok_us = msg.t_us + (DEBOUNCE_MS * 1000);
            }
        }
    }
}
