#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/ledc.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include <lwip/inet.h>

// --- Definición de pines ---
#define LED_PIN             2
#define LED_PIN4             4
#define INPUT_DIG_1         12
#define INPUT_DIG_2         13
#define INPUT_ANALOG        34
#define INPUT_ANALOG_2      35
#define OUTPUT_ANALOG       25
#define OUTPUT_PWM          26

// --- CORRECCIÓN 1: Definir el canal DAC correcto ---
#define OUTPUT_ANALOG_CHANNEL DAC_CHANNEL_1  // GPIO25 = DAC_CHANNEL_1
#define PWM_CHANNEL          LEDC_CHANNEL_0
#define PWM_TIMER            LEDC_TIMER_0
#define PWM_FREQUENCY        5000
#define PWM_RESOLUTION       LEDC_TIMER_8_BIT

#define BLINK_DELAY_MS      2000
#define WIFI_SSID           "*****"
#define WIFI_PASSWORD       "*****"


// --- CORRECCIÓN 2: Variable global para el valor del DAC ---
static int g_dac_value = 100;
static int g_pwm_value = 0;

// Declaración de archivos web incrustados
extern const char _binary_index_html_start[];
extern const char _binary_index_html_end[];
extern const char _binary_style_css_start[];
extern const char _binary_style_css_end[];
extern const char _binary_script_js_start[];
extern const char _binary_script_js_end[];

// ---------------------------------------------------------
// HANDLERS
// ---------------------------------------------------------

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, _binary_index_html_start,
                    _binary_index_html_end - _binary_index_html_start);
    return ESP_OK;
}

static esp_err_t css_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, _binary_style_css_start,
                    _binary_style_css_end - _binary_style_css_start);
    return ESP_OK;
}

static esp_err_t js_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, _binary_script_js_start,
                    _binary_script_js_end - _binary_script_js_start);
    return ESP_OK;
}

static esp_err_t get_readings_handler(httpd_req_t *req)
{
    int dig1 = gpio_get_level(INPUT_DIG_1);
    int dig2 = gpio_get_level(INPUT_DIG_2);
    int analog_val = adc1_get_raw(ADC1_CHANNEL_6);
    int analog_val_2 = adc1_get_raw(ADC1_CHANNEL_7);
    
    // --- CORRECCIÓN 2: Usar la variable global ---
    int dac_val = g_dac_value;
    int pwm_val = g_pwm_value;

    char resp[256];
    snprintf(resp, sizeof(resp),
             "{\"dig1\": %d, \"dig2\": %d, \"analog\": %d, \"analog2\": %d, \"dac\": %d, \"pwm\": %d}",
             dig1, dig2, analog_val, analog_val_2, dac_val, pwm_val);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

static esp_err_t control_handler(httpd_req_t *req)
{
    char query[128];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        
        char param[32];
        // Controlar LED
        if (httpd_query_key_value(query, "led", param, sizeof(param)) == ESP_OK) {
            int led_state = atoi(param);
            gpio_set_level(LED_PIN, led_state ? 1 : 0);
            httpd_resp_sendstr(req, "LED actualizado");
            return ESP_OK;
        }
        // Controlar GPIO 4
		if (httpd_query_key_value(query, "led4", param, sizeof(param)) == ESP_OK) {
		    int state = atoi(param);
		    gpio_set_level(LED_PIN4, state ? 1 : 0);
		    httpd_resp_sendstr(req, "LED4 actualizado");
		    return ESP_OK;
		}

        // Controlar DAC
        if (httpd_query_key_value(query, "dac", param, sizeof(param)) == ESP_OK) {
            int dac_value = atoi(param);
            if (dac_value < 100) dac_value = 100;
            if (dac_value > 255) dac_value = 255;
            
            // --- CORRECCIÓN 1: Usar el canal, no el GPIO ---
            dac_output_voltage(OUTPUT_ANALOG_CHANNEL, dac_value);
            
            // --- CORRECCIÓN 2: Guardar el valor en la variable global ---
            g_dac_value = dac_value;
            
            httpd_resp_sendstr(req, "DAC actualizado");
            return ESP_OK;
        }

        // Controlar PWM
        if (httpd_query_key_value(query, "pwm", param, sizeof(param)) == ESP_OK) {
            int pwm_value = atoi(param);
            if (pwm_value < 0) pwm_value = 0;
            if (pwm_value > 255) pwm_value = 255;

            ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, pwm_value);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
            g_pwm_value = pwm_value;

            httpd_resp_sendstr(req, "PWM actualizado");
            return ESP_OK;
        }
    }
    
    httpd_resp_send_404(req);
    return ESP_OK;
}

static esp_err_t ip_get_handler(httpd_req_t *req)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));

    char resp[64];
    snprintf(resp, sizeof(resp), "{\"ip\": \"%s\"}", ip_str);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

// ---------------------------------------------------------
// RUTAS
// ---------------------------------------------------------

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};

static const httpd_uri_t css = {
    .uri       = "/style.css",
    .method    = HTTP_GET,
    .handler   = css_get_handler
};

static const httpd_uri_t js = {
    .uri       = "/script.js",
    .method    = HTTP_GET,
    .handler   = js_get_handler
};

static const httpd_uri_t readings = {
    .uri       = "/analog",
    .method    = HTTP_GET,
    .handler   = get_readings_handler
};

static const httpd_uri_t control = {
    .uri       = "/control",
    .method    = HTTP_GET,
    .handler   = control_handler
};

static const httpd_uri_t ip_uri = {
    .uri       = "/ip",
    .method    = HTTP_GET,
    .handler   = ip_get_handler
};

static void start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_req_hdr_len = 4096;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &css);
        httpd_register_uri_handler(server, &js);
        httpd_register_uri_handler(server, &readings);
        httpd_register_uri_handler(server, &control);
        httpd_register_uri_handler(server, &ip_uri);
        printf("Servidor web iniciado correctamente\n");
    }
}

static void event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("WiFi desconectado, reconectando...\n");
        esp_wifi_connect();
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) data;
        printf("IP obtenida: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        start_webserver();
    }
}

void app_main(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    
     // ====== AQUÍ: CONFIGURACIÓN IP ESTÁTICA ======
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_ip_info_t ip_info;
        IP4_ADDR(&ip_info.ip, 192, 168, 0, 50);      // IP deseada
        IP4_ADDR(&ip_info.gw, 192, 168, 0, 1);        // Gateway
        IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0); // Máscara
        esp_netif_dhcpc_stop(netif);                  // Detener DHCP
        esp_netif_set_ip_info(netif, &ip_info);       // Aplicar IP
    }
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        }
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    // --- CONFIGURACIÓN DE PINES ---
    
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);
    gpio_set_direction(LED_PIN4, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN4, 0);

    gpio_set_direction(INPUT_DIG_1, GPIO_MODE_INPUT);
    gpio_set_pull_mode(INPUT_DIG_1, GPIO_PULLDOWN_ONLY);
    gpio_set_direction(INPUT_DIG_2, GPIO_MODE_INPUT);
    gpio_set_pull_mode(INPUT_DIG_2, GPIO_PULLDOWN_ONLY);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_12);

    ledc_timer_config_t pwm_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = PWM_TIMER,
        .duty_resolution  = PWM_RESOLUTION,
        .freq_hz          = PWM_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&pwm_timer);

    ledc_channel_config_t pwm_channel = {
        .gpio_num       = OUTPUT_PWM,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = PWM_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = PWM_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&pwm_channel);

    // --- CORRECCIÓN 1: Usar el canal DAC correcto ---
    dac_output_enable(OUTPUT_ANALOG_CHANNEL);
    dac_output_voltage(OUTPUT_ANALOG_CHANNEL, 100);

    // --- BUCLE PRINCIPAL ---
    bool led_state = false;
    
    while (1) {
        //led_state = !led_state;
        //gpio_set_level(LED_PIN, led_state ? 1 : 0);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS);
    }
}