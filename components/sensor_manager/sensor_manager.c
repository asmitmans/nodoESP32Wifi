#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "nvs_storage.h"

#include "sensor_manager.h"
#include "wifi_manager.h"
#include "ntp_client.h"
#include "dht22.h"
#include "http_client.h"
#include "task_sensor.h"
#include "task_http_post.h"

static const char *TAG = "SENSOR_MANAGER";

static QueueHandle_t sensor_data_queue;
static QueueHandle_t http_post_result_queue;
static SemaphoreHandle_t http_post_done_semaphore;


QueueHandle_t sensor_manager_get_queue(void) {
    return sensor_data_queue;
}


QueueHandle_t sensor_manager_get_post_queue(void) {
    return http_post_result_queue;
}

// Manejar fallos críticos
static void manejar_fallo(const char *motivo) {
    ESP_LOGE(TAG, "Fallo crítico: %s. Entrando en deep sleep...", motivo);
    esp_sleep_enable_timer_wakeup(MEDICION_INTERVALO_MS * 1000);
    esp_deep_sleep_start();
}

// Inicializar recursos
static void inicializar_recursos() {
    sensor_data_queue = xQueueCreate(1, sizeof(sensor_data_t));
    if (sensor_data_queue == NULL) {
        manejar_fallo("Error creando cola de datos del sensor");
    }

    http_post_result_queue = xQueueCreate(1, sizeof(bool));
    if (http_post_result_queue == NULL) {
        ESP_LOGE(TAG, "Error al crear http_post_result_queue");
    }

    http_post_done_semaphore = xSemaphoreCreateBinary();
    nvs_storage_init();
}

// Enviar datos vía HTTP
static bool enviar_datos_http(sensor_data_t *data) {
    ESP_LOGI(TAG, "Intentando enviar datos...");

    if (xQueueSend(sensor_data_queue, data, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGE(TAG, "Error enviando datos a la cola de HTTP POST.");
        return false;
    }

    xTaskCreate(task_http_post, "task_http_post", 4096, NULL, 5, NULL);

    bool envio_exitoso = false;
    if (!xQueueReceive(http_post_result_queue, &envio_exitoso, pdMS_TO_TICKS(10000))) {
        ESP_LOGW(TAG, "Timeout en HTTP POST. Se guardarán datos en NVS.");
        envio_exitoso = false;
    }

    if (envio_exitoso) {
        ESP_LOGI(TAG, "Datos enviados con éxito.");
    } else {
        ESP_LOGE(TAG, "Error al enviar datos. Se deberían guardar en NVS.");
    }

    return envio_exitoso;
}

// Reenviar datos pendientes desde NVS
void reenviar_datos_pendientes_nvs() {
    sensor_data_t datos_pendientes[MAX_NVS_RECORDS];
    char claves_existentes[MAX_NVS_RECORDS][MAX_KEY_LEN];  
    size_t count = nvs_retrieve_failed_data(datos_pendientes, claves_existentes);

    if (count == 0) {
        ESP_LOGI(TAG, "No hay datos pendientes en NVS.");
        return;
    }

    size_t enviados_con_exito = 0;
    for (size_t i = 0; i < count; i++) {
        if (enviar_datos_http(&datos_pendientes[i])) {
            ESP_LOGI(TAG, "Dato reenviado desde NVS: %s", claves_existentes[i]);
            enviados_con_exito++;
        } else {
            ESP_LOGW(TAG, "Error reenviando dato desde NVS. Se intentará en el próximo ciclo.");
        }
    }

    // Solo eliminamos los datos que se reenviaron con éxito
    for (size_t j = 0; j < enviados_con_exito; j++) {
        esp_err_t err = nvs_delete_key(claves_existentes[j]);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Eliminado registro de NVS: %s", claves_existentes[j]);
        } else {
            ESP_LOGW(TAG, "Error eliminando %s: %s", claves_existentes[j], esp_err_to_name(err));
        }
    }
}

// Manejo de conexión Wi-Fi
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

// Manejo de sincronización NTP
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

            uint64_t timestamp_end = esp_timer_get_time();
            *epoch_time -= (timestamp_end - timestamp_start) / 1000000;
            ESP_LOGI(TAG, "NTP sincronizado. Timestamp ajustado: %llu", *epoch_time);
            return true;
        }

        ESP_LOGW(TAG, "Esperando sincronización NTP... (%d/%d)", i + 1, NTP_MAX_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
    }

    ESP_LOGE(TAG, "No se pudo sincronizar NTP.");
    return false;
}

// Tiempo de espera restante
static uint64_t calcular_tiempo_restante(uint64_t time_start) {
    uint64_t timestamp_end = esp_timer_get_time();
    uint64_t tiempo_transcurrido = (timestamp_end - time_start) / 1000;

    int64_t tiempo_dormir = MEDICION_INTERVALO_MS - tiempo_transcurrido;
    if (tiempo_dormir < 0) tiempo_dormir = 0;

    ESP_LOGI(TAG, "Tiempo de ejecución: %llu ms, durmiendo por %llu ms.", 
             tiempo_transcurrido, tiempo_dormir);
    return tiempo_dormir;
}

// Función principal
void sensor_manager_init() {
    ESP_LOGI(TAG, "Iniciando ciclo de operación...");

    inicializar_recursos();

    xTaskCreate(task_sensor_read, "task_sensor_read", 4096, NULL, 5, NULL);
    sensor_data_t data;
    if (xQueueReceive(sensor_data_queue, &data, pdMS_TO_TICKS(5000)) != pdTRUE) {
        manejar_fallo("Timeout esperando datos del sensor");
    }

    if (!conectar_wifi()) manejar_fallo("No se pudo conectar a Wi-Fi");

    uint64_t time_start = data.timestamp;
    if (!sincronizar_ntp(&data.timestamp, time_start)) {
        data.status_code = 200;
    }

    // Asignar status 300 si el dato debe ser almacenado
    bool envio_exitoso = enviar_datos_http(&data);
    if (!envio_exitoso) {
        data.status_code = 300;
        ESP_LOGW(TAG, "Guardando dato actual en NVS con status 300.");
        esp_err_t err = nvs_store_failed_data(&data);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error al guardar dato en NVS: %s", esp_err_to_name(err));
        }
    } else {
        reenviar_datos_pendientes_nvs();
    }

    uint64_t tiempo_dormir = calcular_tiempo_restante(time_start);
    esp_sleep_enable_timer_wakeup(tiempo_dormir * 1000);
    esp_deep_sleep_start();
}
