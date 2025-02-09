#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_storage.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "sensor_manager.h"

static const char *TAG = "NVS_STORAGE";

esp_err_t nvs_storage_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS necesita reinicialización, borrando...");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t nvs_store_failed_data(const sensor_data_t *data) {
    nvs_handle_t nvs_handle;
    sensor_data_t buffer[MAX_NVS_RECORDS];
    size_t stored_count = 0;

    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;

    size_t required_size = sizeof(buffer);
    err = nvs_get_blob(nvs_handle, NVS_KEY, buffer, &required_size);
    if (err == ESP_OK) {
        stored_count = required_size / sizeof(sensor_data_t);
    }

    if (stored_count < MAX_NVS_RECORDS) {
        buffer[stored_count] = *data;
        stored_count++;
    } else {
        for (int i = 1; i < MAX_NVS_RECORDS; i++) {
            buffer[i - 1] = buffer[i];
        }
        buffer[MAX_NVS_RECORDS - 1] = *data;
    }

    err = nvs_set_blob(nvs_handle, NVS_KEY, buffer, stored_count * sizeof(sensor_data_t));
    if (err == ESP_OK) {
        nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    return err;
}

size_t nvs_retrieve_failed_data(sensor_data_t *buffer) {
    nvs_handle_t nvs_handle;
    size_t stored_count = 0;

    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return 0;

    size_t required_size = MAX_NVS_RECORDS * sizeof(sensor_data_t);
    err = nvs_get_blob(nvs_handle, NVS_KEY, buffer, &required_size);
    if (err == ESP_OK) {
        stored_count = required_size / sizeof(sensor_data_t);
    }

    nvs_close(nvs_handle);
    return stored_count;
}

esp_err_t nvs_clear_failed_data(size_t count) {
    if (count == 0) return ESP_OK;

    nvs_handle_t nvs_handle;
    sensor_data_t buffer[MAX_NVS_RECORDS];

    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;

    size_t stored_count = nvs_retrieve_failed_data(buffer);
    if (stored_count > count) {
        for (size_t i = count; i < stored_count; i++) {
            buffer[i - count] = buffer[i];
        }
        stored_count -= count;
        err = nvs_set_blob(nvs_handle, NVS_KEY, buffer, stored_count * sizeof(sensor_data_t));
    } else {
        err = nvs_erase_key(nvs_handle, NVS_KEY);
    }

    if (err == ESP_OK) {
        nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    return err;
}
