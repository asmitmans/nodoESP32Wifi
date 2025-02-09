#include "ntp_client.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include <sys/time.h>
#include <time.h>

#define TAG "NTP_CLIENT"

static bool ntp_synced = false;
static time_t last_sync_time = 0;


// Callback de sincronización
static void time_sync_notification_cb(struct timeval *tv) {
    ntp_synced = true;
    time(&last_sync_time);  // Guarda el tiempo de la última sincronización
    ESP_LOGI(TAG, "Sincronización NTP confirmada.");
}

// Inicializa SNTP
static void initialize_sntp(const char *server) {
    if (!esp_sntp_enabled()) {
        ESP_LOGI(TAG, "Iniciando SNTP...");
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, server);
        esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
        esp_sntp_init();
    }
}

// Inicialización al arranque
void ntp_client_init(const char *ntp_server) {
    initialize_sntp(ntp_server);

    time_t now;
    struct tm timeinfo = {0};
    int retry = 0, max_retries = 10;

    while (timeinfo.tm_year < (2020 - 1900) && ++retry < max_retries) {
        ESP_LOGI(TAG, "Esperando sincronización NTP... (%d/%d)", retry, max_retries);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        gmtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year >= (2020 - 1900)) {
        ESP_LOGI(TAG, "Sincronización NTP exitosa (UTC): %s", asctime(&timeinfo));
        ntp_synced = true;
        last_sync_time = now;
    } else {
        ESP_LOGW(TAG, "No se pudo sincronizar la hora con NTP.");
    }
}

// Verifica si necesita re-sincronización (cada 1 hora)
bool ntp_client_needs_resync(void) {
    time_t now;
    time(&now);
    return (difftime(now, last_sync_time) >= NTP_SYNC_INTERVAL);
}

// Sincronización manual si es necesario
void ntp_client_resync(const char *ntp_server) {
    if (ntp_client_needs_resync()) {
        ESP_LOGI(TAG, "Re-sincronizando NTP...");
        esp_sntp_stop();
        initialize_sntp(ntp_server);
    }
}

uint64_t ntp_client_get_epoch() {
    struct timeval tv;
    gettimeofday(&tv, NULL);  // Obtiene la hora actual

    if (tv.tv_sec == 0) {
        ESP_LOGE(TAG, "No se pudo obtener el tiempo desde NTP.");
        return 0;  // Indicar error
    }

    return (uint64_t)tv.tv_sec;  // Devolver el tiempo UNIX (segundos)
}

bool ntp_client_is_synced(void) {
    return ntp_synced;
}
