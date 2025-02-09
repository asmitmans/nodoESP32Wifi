#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "sensor_manager.h"
#include "wifi_manager.h"
#include "ntp_client.h"
#include "dht22.h"
#include "http_client.h"
#include "task_sensor.h"
#include "task_http_post.h"
#include "nvs_storage.h"


static const char *TAG = "SENSOR_MANAGER";

static QueueHandle_t sensor_data_queue;
static SemaphoreHandle_t http_post_done_semaphore;


static void manejar_fallo() {
    ESP_LOGE(TAG, "Error crítico. Entrando en deep sleep...");
    esp_sleep_enable_timer_wakeup(MEDICION_INTERVALO_MS * 1000);
    esp_deep_sleep_start();
}

static void inicializar_recursos() {
    sensor_data_queue = xQueueCreate(1, sizeof(sensor_data_t));
    if (sensor_data_queue == NULL) {
        manejar_fallo();
    }

    http_post_done_semaphore = xSemaphoreCreateBinary();
}

// Enviar datos HTTP con reintento
static bool enviar_datos_http(sensor_data_t *data) {
    if (xQueueSend(sensor_data_queue, data, pdMS_TO_TICKS(5000)) != pdTRUE) {
        return false;
    }

    xTaskCreate(task_http_post, "task_http_post", 4096, NULL, 5, NULL);

    return xSemaphoreTake(http_post_done_semaphore, pdMS_TO_TICKS(10000)) == pdTRUE;
}

static uint64_t calcular_tiempo_restante(uint64_t time_start) {
    uint64_t timestamp_end = esp_timer_get_time();
    uint64_t tiempo_transcurrido = (timestamp_end - time_start) / 1000; // Milisegundos

    int64_t tiempo_dormir = MEDICION_INTERVALO_MS - tiempo_transcurrido;
    if (tiempo_dormir < 0) tiempo_dormir = 0; // Evitar valores negativos

    ESP_LOGI(TAG, "Tiempo de ejecución: %llu ms, durmiendo por %llu ms.", 
             tiempo_transcurrido, tiempo_dormir);

    return tiempo_dormir;
}

// Conectar Wi-Fi
static bool conectar_wifi() {
    ESP_LOGI(TAG, "Conectando a Wi-Fi...");
    wifi_init();
    
    for (int i = 0; i < WIFI_MAX_RETRIES; i++) {
        if (is_wifi_connected()) {
            ESP_LOGI(TAG, "Wi-Fi conectado.");
            return true;
        }
        ESP_LOGW(TAG, "Esperando conexión Wi-Fi... (%d/%d)", i + 1, WIFI_MAX_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
    }

    ESP_LOGE(TAG, "No se pudo conectar a Wi-Fi.");
    return false;
}

// Sincronizar NTP y ajustar timestamp
static bool sincronizar_ntp(uint64_t *epoch_time, uint64_t timestamp_start) {
    ESP_LOGI(TAG, "Sincronizando NTP...");
    ntp_client_init(NTP_SERVER);

    for (int i = 0; i < NTP_MAX_RETRIES; i++) {
        if (ntp_client_is_synced()) {
            *epoch_time = ntp_client_get_epoch();
            if (*epoch_time == 0) {
                ESP_LOGE(TAG, "Error obteniendo tiempo NTP.");
                return false;
            }

            // Ajustar timestamp usando la diferencia entre tiempos
            uint64_t timestamp_end = esp_timer_get_time();
            *epoch_time -= (timestamp_end - timestamp_start) / 1000000; // Convertir a segundos

            ESP_LOGI(TAG, "NTP sincronizado. Timestamp ajustado: %llu", *epoch_time);
            return true;
        }

        ESP_LOGW(TAG, "Esperando sincronización NTP... (%d/%d)", i + 1, NTP_MAX_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
    }

    ESP_LOGE(TAG, "No se pudo sincronizar NTP.");
    return false;
}

QueueHandle_t sensor_manager_get_queue(void) {
    return sensor_data_queue;
}

SemaphoreHandle_t sensor_manager_get_semaphore(void) {
    return http_post_done_semaphore;
}

// **Ciclo principal de operación**
void sensor_manager_init() {
    ESP_LOGI(TAG, "Iniciando ciclo de operación...");

    inicializar_recursos();
    nvs_storage_init();

    // 1. Lectura del sensor
    xTaskCreate(task_sensor_read, "task_sensor_read", 4096, NULL, 5, NULL);
    sensor_data_t data;
    if (xQueueReceive(sensor_data_queue, &data, pdMS_TO_TICKS(5000)) != pdTRUE) {
        manejar_fallo();
    }

    // 2. Conectar Wi-Fi
    if (!conectar_wifi()) manejar_fallo();

    // 3. Sincronizar NTP
    uint64_t time_start = data.timestamp;
    if (!sincronizar_ntp(&data.timestamp, time_start)) {
        data.status_code = 200;
    }

    // 4. Enviar primero el dato actual
    if (!enviar_datos_http(&data)) {
        nvs_store_failed_data(&data);
    }

    // 5. Recuperar y enviar datos previos después del actual**
    sensor_data_t datos_pendientes[MAX_NVS_RECORDS];
    size_t count = nvs_retrieve_failed_data(datos_pendientes);

    for (size_t i = 0; i < count; i++) {
        datos_pendientes[i].status_code = 300;
        enviar_datos_http(&datos_pendientes[i]);
    }

    // 6. Eliminar los datos reenviados correctamente**
    nvs_clear_failed_data(count);
    
    // 7. Dormir
    uint64_t tiempo_dormir = calcular_tiempo_restante(time_start);
    esp_sleep_enable_timer_wakeup(tiempo_dormir * 1000);
    esp_deep_sleep_start();
}
