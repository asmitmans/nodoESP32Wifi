#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define DEVICE_ID "ESP32-001"
static const int RETRY_DELAY_MS = 1000;
static const int WIFI_MAX_RETRIES = 10;
static const int NTP_MAX_RETRIES = 5;
static const uint64_t MEDICION_INTERVALO_MS = 60000; // 1 minuto (ajustable)

typedef struct {
    uint64_t timestamp;  
    float temperature;
    float humidity;
    int status_code;
} sensor_data_t;

void sensor_manager_init(void);
bool ntp_client_is_synced();
uint64_t ntp_client_get_epoch(); 
QueueHandle_t sensor_manager_get_queue(void);
QueueHandle_t sensor_manager_get_post_queue(void);
SemaphoreHandle_t sensor_manager_get_semaphore(void);

#endif // SENSOR_MANAGER_H