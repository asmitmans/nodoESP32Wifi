#include "wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_storage.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "sdkconfig.h"
#include <string.h>

static const char *TAG = "WIFI_MANAGER";
static bool wifi_connected = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                ESP_LOGI(TAG, "Intentando conectar al Wi-Fi...");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                wifi_connected = false;
                ESP_LOGW(TAG, "Desconectado. Reintentando...");
                esp_wifi_connect(); // Reintento automático
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_connected = true;
        ESP_LOGI(TAG, "Conectado y obtuvo IP.");
    }
}

bool is_wifi_connected() {
    return wifi_connected;
}

void wifi_init() {
    char ssid[32] = {0};
    char password[64] = {0};

    // Obtener credenciales desde NVS
    if (nvs_get_string("wifi_ssid", ssid, sizeof(ssid)) != ESP_OK) {
        ESP_LOGW(TAG, "SSID no encontrado en NVS, usando valor por defecto.");
        strcpy(ssid, CONFIG_WIFI_SSID);
    }

    if (nvs_get_string("wifi_pass", password, sizeof(password)) != ESP_OK) {
        ESP_LOGW(TAG, "Contraseña no encontrada en NVS, usando valor por defecto.");
        strcpy(password, CONFIG_WIFI_PASSWORD);
    }

    ESP_LOGI(TAG, "Conectando a SSID: %s", ssid);

    // Inicializar NVS
    nvs_storage_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Configurar Wi-Fi con credenciales obtenidas
    wifi_config_t wifi_config = { .sta = {} };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Wi-Fi inicializado con SSID: %s", ssid);
}

void wifi_set_credentials(const char *ssid, const char *password) {
    if (ssid && password) {
        nvs_set_string("wifi_ssid", ssid);
        nvs_set_string("wifi_pass", password);
        ESP_LOGI(TAG, "Credenciales Wi-Fi guardadas en NVS.");
    }
}
