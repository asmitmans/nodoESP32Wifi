#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_storage.h"
#include "nvs_flash.h"
#include "nvs.h"


static const char *TAG = "NVS_STORAGE";


esp_err_t nvs_storage_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NVS inicializado correctamente.");
    } else {
        ESP_LOGE(TAG, "Error inicializando NVS: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_set_value(const char *key, int32_t value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_i32(handle, key, value);
        if (err == ESP_OK) {
            nvs_commit(handle);
            ESP_LOGI(TAG, "Valor %ld almacenado en NVS con clave: %s", value, key);
        }
        nvs_close(handle);
    }
    return err;
}

esp_err_t nvs_get_value(const char *key, int32_t *value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_get_i32(handle, key, value);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Valor %ld recuperado de NVS con clave: %s", *value, key);
        }
        nvs_close(handle);
    }
    return err;
}

esp_err_t nvs_set_string(const char *key, const char *value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, key, value);
        if (err == ESP_OK) {
            err = nvs_commit(handle);
        }
        nvs_close(handle);
    }
    return err;
}

esp_err_t nvs_get_string(const char *key, char *value, size_t max_len) {
    nvs_handle_t handle;
    size_t required_size = max_len;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_get_str(handle, key, value, &required_size);
        nvs_close(handle);
    }
    return err;
}

esp_err_t nvs_delete_key(const char *key) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_erase_key(handle, key);
        if (err == ESP_OK) {
            nvs_commit(handle);
            ESP_LOGI(TAG, "Clave eliminada de NVS: %s", key);
        } else {
            ESP_LOGW(TAG, "No se pudo eliminar la clave de NVS: %s", key);
        }
        nvs_close(handle);
    }
    return err;
}

// Guardar múltiples datos en NVS con claves únicas failed_data_X
esp_err_t nvs_store_failed_data(const sensor_data_t *data) {
    ESP_LOGW(TAG, "Guardando datos en NVS...");
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    
    if (err == ESP_OK) {
        uint32_t write_index = 0;
        size_t required_size = sizeof(write_index);

        // Leer el índice de escritura desde NVS
        esp_err_t check = nvs_get_u32(handle, "write_index", &write_index);
        if (check == ESP_ERR_NVS_NOT_FOUND) {
            write_index = 0; // Si no existe, inicializarlo
        }

        // Generar clave para la nueva posición FIFO
        char key[MAX_KEY_LEN];
        snprintf(key, MAX_KEY_LEN, "failed_data_%ld", write_index % 100);

        // Guardar el nuevo dato
        err = nvs_set_blob(handle, key, data, sizeof(sensor_data_t));
        if (err == ESP_OK) {
            // Incrementar y almacenar el índice, asegurando que no se salga del rango
            write_index = (write_index + 1) % MAX_NVS_RECORDS;
            nvs_set_u32(handle, "write_index", write_index);
            nvs_commit(handle);
            ESP_LOGI(TAG, "Dato guardado en %s, próximo índice: %ld", key, write_index % 100);
        } else {
            ESP_LOGE(TAG, "Error guardando en %s", key);
        }

        nvs_close(handle);
    }

    return err;
}


// Recuperar datos fallidos
size_t nvs_retrieve_failed_data(sensor_data_t *buffer, char claves_existentes[MAX_NVS_RECORDS][MAX_KEY_LEN]) {
    nvs_handle_t handle;
    size_t count = 0;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);

    if (err == ESP_OK) {
        uint32_t write_index = 0;
        size_t required_size = sizeof(write_index);
        esp_err_t check = nvs_get_u32(handle, "write_index", &write_index);
        if (check == ESP_ERR_NVS_NOT_FOUND) {
            write_index = 0;
        }

        // Recorrer TODAS las posibles posiciones en el buffer FIFO
        for (int i = 0; i < MAX_NVS_RECORDS; i++) {
            char key[MAX_KEY_LEN];
            snprintf(key, MAX_KEY_LEN, "failed_data_%d", i);

            size_t required_size = sizeof(sensor_data_t);
            if (nvs_get_blob(handle, key, &buffer[count], &required_size) == ESP_OK) {
                snprintf(claves_existentes[count], MAX_KEY_LEN, "%s", key);
                count++;
            }
        }
        nvs_close(handle);
    }
    return count;
}

// Borrar datos fallidos
esp_err_t nvs_clear_failed_data(size_t count) {
    if (count == 0) return ESP_OK;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    
    if (err == ESP_OK) {
        for (int i = 0; i < count; i++) {
            char key[MAX_KEY_LEN];
            snprintf(key, MAX_KEY_LEN, "failed_data_%d", i % 100);
            nvs_delete_key(key);
        }
        ESP_LOGI(TAG, "Eliminados %d registros de NVS.", count);
        nvs_close(handle);
    }
    return err;
}

// Borrar todo el almacenamiento NVS
esp_err_t nvs_clear_all() {
    esp_err_t err = nvs_flash_erase();
    if (err == ESP_OK) {
        err = nvs_flash_init();
    }
    return err;
}
