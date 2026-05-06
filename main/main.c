#include "freertos/idf_additions.h"
#include "portmacro.h"
#include <stdbool.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#define LED_PIN         2
#define BLINK_DELAY_MS  2000
#define WIFI_SSID       "Familia Tola"
#define WIFI_PASSWORD   "78312bea"

const char* index_html =
"<!DOCTYPE html>"
"<html>"
"<head>"
"<title>ESP32 Web Server</title>"
"<meta charset=\"UTF-8\">"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"<style>"
"body { font-family: Arial; text-align: center; margin-top: 50px; background: #0a0e1a; color: white; }"
"h1 { color: #00e5ff; }"
"</style>"
"</head>"
"<body>"
"<h1>¡Hola Mundo desde ESP32!</h1>"
"<p>Servidor web funcionando correctamente</p>"
"</body>"
"</html>";

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};

static void start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        printf("Servidor web iniciado\n");
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
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    // CORREGIDO: Registrar ambos eventos
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
    
    wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASSWORD } };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    bool led_state = false;
    
    while (1) {
        led_state = !led_state;
        if (led_state) {
            gpio_set_level(LED_PIN, 1);
        } else {
            gpio_set_level(LED_PIN, 0);
        }
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS);
    }
}