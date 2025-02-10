#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"


#include "task_http_post.h"
#include "http_client.h"
#include "sensor_manager.h"


static const char *TAG = "TASK_HTTP_POST";


void task_http_post(void *pvParameters) {
    sensor_data_t data;
    QueueHandle_t sensor_data_queue = sensor_manager_get_queue();

    // **Esperar hasta recibir datos de la cola**
    if (xQueueReceive(sensor_data_queue, &data, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGE(TAG, "Timeout esperando datos para HTTP POST.");
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Enviando datos...");

    char json_data[256];
    snprintf(json_data, sizeof(json_data), 
         "{"
         "\"device_id\": \"%s\", "
         "\"timestamp\": %llu, "
         "\"temperature\": %.2f, "
         "\"humidity\": %.2f, "
         "\"status_code\": %d"
         "}", 
         DEVICE_ID, 
         data.timestamp, 
         data.temperature, 
         data.humidity, 
         data.status_code
    );

    // **Reintentos de envío**
    bool success = false;
    for (int i = 0; i < CONFIG_HTTP_POST_RETRIES; i++) {
        if (http_client_post(json_data)) {
            ESP_LOGI(TAG, "Datos enviados correctamente.");
            success = true;
            break;
        }
        ESP_LOGW(TAG, "Error al enviar datos HTTP. Reintentando... (%d/%d)", i + 1, CONFIG_HTTP_POST_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_HTTP_POST_RETRY_DELAY));
    }

    if (!success) {
        ESP_LOGE(TAG, "Error crítico: Fallaron todos los intentos de envío.");
        data.status_code = 300; // Código de error de envío
    }

    // **Notificar que la tarea ha finalizado**
    SemaphoreHandle_t http_post_done_semaphore = sensor_manager_get_semaphore();
    xSemaphoreGive(http_post_done_semaphore);

    vTaskDelete(NULL);  // Terminar la tarea
}
