#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "TAM_MDC.h"

#define horario_bt 4
#define antihorario_bt 5

void configuracao_botao(int botao) {
    gpio_config_t io_conf = {
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << botao),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

int estado_anterior_horario = 0;
int estado_anterior_antihorario = 0;

int botao_pressionado(int pino, int *estado_anterior) {
    int estado_atual = gpio_get_level(pino);

    // Detecta se o botão mudou de solto (0) para pressionado (1)
    if (estado_atual == 1 && *estado_anterior == 0) {
        vTaskDelay(pdMS_TO_TICKS(80)); // Filtro de tempo (debounce), evita falso acionamtno
        if (gpio_get_level(pino) == 1) { // Confirma se o botão continua pressionado
            *estado_anterior = 1;
            return 1; // Retorna verdadeiro (botão pressionado)
        }
    }
    // Reseta o estado quando o botão é solto
    if (estado_atual == 0) {
        *estado_anterior = 0;
    }
    return 0;
}

void app_main(void) {
    config_dc();
    configuracao_botao(horario_bt);
    configuracao_botao(antihorario_bt);

    while (1) {
        // Lê o nível elétrico atual dos pinos (0 = Pressionado, 1 = Solto)
        int estado_horario = gpio_get_level(horario_bt);
        int estado_antihorario = gpio_get_level(antihorario_bt);

        // Se APENAS o botão horário estiver pressionado (0)
        if (estado_horario == 0 && estado_antihorario == 1) {
            horario_dc(1023); // Mantém ligado na velocidade máxima
        }
        // Se APENAS o botão anti-horário estiver pressionado (0)
        else if (estado_antihorario == 0 && estado_horario == 1) {
            anti_horario_dc(1023); // Mantém ligado na velocidade máxima
        }
        // Se nenhum estiver pressionado (1 e 1) ou os dois juntos (0 e 0)
        else {
            freiar_dc(); // Freia o motor de forma contínua
        }

        // Delay essencial para o Watchdog do ESP-IDF e estabilidade dos botões
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}