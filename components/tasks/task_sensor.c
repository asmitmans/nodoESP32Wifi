#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "dht22.h"
#include "task_sensor.h"
#include "sensor_manager.h"

static const char *TAG = "TASK_SENSOR";
static const int SENSOR_RETRY_COUNT = 3;
static const int SENSOR_RETRY_DELAY_MS = 2000;

// **Tarea de lectura del sensor**
void task_sensor_read(void *pvParameters) {
    sensor_data_t data;
    data.timestamp = esp_timer_get_time();      // Guardamos el tiempo de inicio
    data.status_code = 0;

    ESP_LOGI(TAG, "Iniciando lectura del sensor DHT22...");
    dht22_init();
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Intentar leer el sensor con reintentos
    bool success = false;
    for (int i = 0; i < SENSOR_RETRY_COUNT; i++) {
        if (dht22_read(&data.temperature, &data.humidity)) {
            success = true;
            data.status_code = 200;
            break;
        }
        ESP_LOGW(TAG, "Error al leer sensor DHT22. Reintentando... (%d/%d)", i + 1, SENSOR_RETRY_COUNT);
        vTaskDelay(pdMS_TO_TICKS(SENSOR_RETRY_DELAY_MS));
    }

    if (!success) {
        ESP_LOGE(TAG, "Fallo en la lectura del sensor.");
        data.temperature = -999.0;
        data.humidity = -999.0;
        data.status_code = 100;
    }

    // Enviar datos a la cola
    QueueHandle_t queue = sensor_manager_get_queue();
    if (xQueueSend(queue, &data, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Error al enviar datos a la cola.");
    } else {
        ESP_LOGI(TAG, "Lectura enviada a la cola: Temp = %.2f°C, Humedad = %.2f%%, Status: %d", 
                data.temperature, data.humidity, data.status_code);
    }

    vTaskDelete(NULL);  // Finalizar la tarea
}
