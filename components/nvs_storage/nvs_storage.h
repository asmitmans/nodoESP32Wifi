#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include "esp_err.h"
#include "sensor_manager.h"  // Para usar `sensor_data_t`

#define NVS_NAMESPACE "storage"
#define NVS_KEY "failed_data"
#define MAX_NVS_RECORDS 5

esp_err_t nvs_storage_init();
esp_err_t nvs_store_failed_data(const sensor_data_t *data);
size_t nvs_retrieve_failed_data(sensor_data_t *buffer);
esp_err_t nvs_clear_failed_data(size_t count);

#endif // NVS_STORAGE_H
