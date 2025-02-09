#include <stdbool.h>
#include "dht22.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "dht.h"

static const char *TAG = "DHT22";


void dht22_init() {
    ESP_LOGI(TAG, "Inicializando DHT22...");
    gpio_set_pull_mode(DHT_GPIO, GPIO_PULLUP_ONLY);
}

bool dht22_read(float *temperature, float *humidity) {
    if (dht_read_float_data(SENSOR_TYPE, DHT_GPIO, humidity, temperature) == ESP_OK) {
        return true;
    } else {
        ESP_LOGW(TAG, "Error al leer el sensor DHT22");
        return false;
    }
}