#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"

// ============================================================================
// CONFIGURAÇÕES DO WI-FI E SERVIDORES
// ============================================================================
#define WIFI_SSID      "Thiago AM"
#define WIFI_PASS      "Thilipe13"

// ============================================================================
// MAPEAMENTO DE PINOS V5.0
// ============================================================================
#define GPIO_IN1            26  // Driver L9110S IN1
#define GPIO_IN2            27  // Driver L9110S IN2
#define GPIO_SERVO          25  // Servo Garra
#define GPIO_LM393_SPEED    18  // Sensor de Velocidade LM393
#define GPIO_SW_HOME        32  // Fim de Curso Origem (E1)
#define GPIO_SW_END         33  // Fim de Curso Fim de Linha (E3)
#define GPIO_SENSOR_OPTICO  34  // Sensor Industrial G18-3B2PC

// Configurações de PWM
#define MOTOR_PWM_TIMER     LEDC_TIMER_0
#define MOTOR_PWM_MODE      LEDC_LOW_SPEED_MODE
#define SERVO_PWM_TIMER     LEDC_TIMER_1
#define SERVO_PWM_MODE      LEDC_LOW_SPEED_MODE

#define SERVO_POS_ABERTO    1638  // ~0°
#define SERVO_POS_FECHADO   6553  // ~90°

// ============================================================================
// VARIÁVEIS GLOBAIS DE ESTADO E GÊMEO DIGITAL
// ============================================================================
int g_encoder_pulses = 0;
int g_motor_direcao_avancando = 1;
int motor_status = 0; // 0 = Desligado, 1 = Ligado

static httpd_handle_t server = NULL;

// ============================================================================
// INICIALIZAÇÃO DO WI-FI (STA MODE) - CORRIGIDO
// ============================================================================
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { // CORRIGIDO AQUI (GOT_IP)
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        // Imprime o IP no monitor serial
        printf("\n=============================================\n");
        printf("Wi-Fi Conectado com Sucesso!\n");
        printf("ENDERECO IP DO ESP32: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        printf("Cole este IP no gemeo.html: http://" IPSTR "/status\n", IP2STR(&event->ip_info.ip));
        printf("=============================================\n\n");
    }
}

void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL); // CORRIGIDO AQUI (GOT_IP)

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}
// ============================================================================
// SERVIDORES HTTP DO GÊMEO DIGITAL
// ============================================================================
static esp_err_t telemetria_handler(httpd_req_t *req) {
    char json_response[128];
    sprintf(json_response, "{\"motor\":%d, \"direcao\":%d, \"pulsos\":%d}", 
            motor_status, g_motor_direcao_avancando, g_encoder_pulses);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
}

void start_web_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_uri_t uri_telemetria = {
        .uri      = "/status",
        .method   = HTTP_GET,
        .handler  = telemetria_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_telemetria);
    }
}

// ============================================================================
// CONFIGURAÇÃO DOS PINOS DA PLACA
// ============================================================================
void configuracao_pino_entrada(int pino) {
    gpio_config_t io_conf = {
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << pino),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

void configuracao_pwms(void) {
    // Timer do Motor
    ledc_timer_config_t timer_motor = {
        .speed_mode      = MOTOR_PWM_MODE,
        .timer_num       = MOTOR_PWM_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_motor);

    ledc_channel_config_t ch_a = {
        .speed_mode = MOTOR_PWM_MODE, .channel = LEDC_CHANNEL_0,
        .timer_sel = MOTOR_PWM_TIMER, .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_IN1, .duty = 0, .hpoint = 0
    };
    ledc_channel_config(&ch_a);

    ledc_channel_config_t ch_b = {
        .speed_mode = MOTOR_PWM_MODE, .channel = LEDC_CHANNEL_1,
        .timer_sel = MOTOR_PWM_TIMER, .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_IN2, .duty = 0, .hpoint = 0
    };
    ledc_channel_config(&ch_b);

    // Timer do Servo
    ledc_timer_config_t timer_servo = {
        .speed_mode      = SERVO_PWM_MODE,
        .timer_num       = SERVO_PWM_TIMER,
        .duty_resolution = LEDC_TIMER_16_BIT,
        .freq_hz         = 50,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_servo);

    ledc_channel_config_t ch_servo = {
        .speed_mode = SERVO_PWM_MODE, .channel = LEDC_CHANNEL_2,
        .timer_sel = SERVO_PWM_TIMER, .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_SERVO, .duty = SERVO_POS_ABERTO, .hpoint = 0
    };
    ledc_channel_config(&ch_servo);
}

// ============================================================================
// PONTO DE ENTRADA PRINCIPAL (app_main)
// ============================================================================
void app_main(void) {
    // 1. Inicializa NVS (Necessário para a memória Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Configura Entradas e PWMs
    configuracao_pino_entrada(GPIO_SW_HOME);
    configuracao_pino_entrada(GPIO_SW_END);
    configuracao_pino_entrada(GPIO_LM393_SPEED);
    configuracao_pwms();

    // 3. Conecta ao Wi-Fi e inicia o Servidor do Gêmeo Digital
    wifi_init_sta();
    start_web_server();

    int estado_anterior_lm393 = gpio_get_level(GPIO_LM393_SPEED);

    // 4. Loop Principal
    while (1) {
        // Leitura contínua dos pulsos do LM393
        int estado_atual_lm393 = gpio_get_level(GPIO_LM393_SPEED);
        if (estado_atual_lm393 == 1 && estado_anterior_lm393 == 0) {
            if (g_motor_direcao_avancando == 1) {
                g_encoder_pulses++;
            } else {
                g_encoder_pulses--;
            }
        }
        estado_anterior_lm393 = estado_atual_lm393;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}