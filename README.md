# Proyecto IoT: ESP32 con Web Server Integrado

## Descripción del Proyecto
Sistema embebido basado en ESP32 que controla un LED y expone un servidor web para monitoreo y control remoto. El dispositivo se conecta a WiFi y permite interactuar con el hardware a través de una interfaz web.

## Características Actuales
- ✅ Conexión automática a red WiFi
- ✅ Control de LED (encendido/apagado cada 2 segundos)
- ✅ Obtención y almacenamiento de IP asignada
- ✅ Salida por puerto serial con estado del LED e IP

## Próximas Implementaciones
- 🔲 Servidor web en el ESP32
- 🔲 Página HTML con botones de control
- 🔲 API REST para comandar el LED
- 🔲 Visualización de estado en tiempo real
- 🔲 Comunicación MQTT (futuro)

## Estructura del Código Actual

### main.c
```c
// Configuración WiFi y LED
#define LED_PIN         2
#define WIFI_SSID       "TU_WIFI"
#define WIFI_PASSWORD   "TU_CLAVE"

// Variables globales
char mi_ip[16];  // Almacena la IP asignada
bool led_state;   // Estado actual del LED

// Manejador de eventos WiFi
static void event_handler() {
    - Conexión a WiFi
    - Reconexión automática
    - Captura de IP
}

// ESP-IDF (Espressif IDE) - Blink LED + Serial
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Configuración
#define LED_PIN     GPIO_NUM_2      // LED_BUILTIN
#define BLINK_DELAY 2000 / portTICK_PERIOD_MS

static const char *TAG = "blink_example";

// Función toggle LED
static void blink_task(void *arg) {
    bool led_state = false;
    
    // Configurar GPIO
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    while (1) {
        // Toggle LED
        led_state = !led_state;
        gpio_set_level(LED_PIN, led_state);
        
        // Mensaje Serial
        if (led_state) {
            ESP_LOGI(TAG, "💡 LED: ON");
        } else {
            ESP_LOGI(TAG, "💡 LED: OFF");
        }
        
        // Delay 2 segundos
        vTaskDelay(BLINK_DELAY);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "🚀 ESP32 Blink ESP-IDF INICIADO");
    
    // Crear tarea blink
    xTaskCreate(&blink_task, "blink_task", 2048, NULL, 5, NULL);
}