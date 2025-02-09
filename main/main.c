#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "sensor_manager.h"


static const char *TAG = "MAIN";


void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema...");
    
    sensor_manager_init();
    
    ESP_LOGI(TAG, "Despues de sensor_manager_init()");
}