#ifndef TAM_MDC_H
#define TAM_MDC_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#define IN1 25  
#define IN2 26  

#define LEDC_CHANNEL_IN1    LEDC_CHANNEL_0
#define LEDC_CHANNEL_IN2    LEDC_CHANNEL_1
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES       LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY      (5000)

void config_dc() {
    // 1. Configurar o Timer do LEDC
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 2. Configurar o Canal para o pino IN1
    ledc_channel_config_t ledc_ch_in1 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_IN1,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = IN1,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_ch_in1);

    // 3. Configurar o Canal para o pino IN2
    ledc_channel_config_t ledc_ch_in2 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_IN2,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = IN2,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_ch_in2);
}

void horario_dc(int velocidade){
    // Alterado de IN1/IN2 para os canais corretos do LEDC (0 e 1)
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_IN1, velocidade);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_IN2, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_IN1);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_IN2);
}

void anti_horario_dc(int velocidade){
    // Alterado de IN1/IN2 para os canais corretos do LEDC (0 e 1)
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_IN1, 0);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_IN2, velocidade);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_IN1);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_IN2);
}

void freiar_dc(){
    // Alterado de IN1/IN2 para os canais corretos do LEDC (0 e 1)
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_IN1, 0);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_IN2, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_IN1);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_IN2);
}


#endif