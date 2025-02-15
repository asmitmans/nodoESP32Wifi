#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include "esp_err.h"
#include "sensor_manager.h"     // Para usar `sensor_data_t`

#define NVS_NAMESPACE "storage"
#define MAX_NVS_RECORDS 5       // Máximo 10 registros permitidos
#define MAX_KEY_LEN 16          // Espacio suficiente para "failed_data_999"

#if MAX_NVS_RECORDS > 10
    #error "MAX_NVS_RECORDS no puede ser mayor a 10"
#endif

// Inicializa NVS
esp_err_t nvs_storage_init();

esp_err_t nvs_set_value(const char *key, int32_t value);
esp_err_t nvs_get_value(const char *key, int32_t *value);

esp_err_t nvs_set_string(const char *key, const char *value);
esp_err_t nvs_get_string(const char *key, char *value, size_t max_len);

// Guarda y recupera datos fallidos
esp_err_t nvs_store_failed_data(const sensor_data_t *data);
size_t nvs_retrieve_failed_data(sensor_data_t *buffer, char claves_existentes[MAX_NVS_RECORDS][MAX_KEY_LEN]);

esp_err_t nvs_clear_failed_data(size_t count);

// Elimina una clave específica
esp_err_t nvs_delete_key(const char *key);

// Borra completamente los datos de NVS
esp_err_t nvs_clear_all();

#endif // NVS_STORAGE_H
