#include "wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h" 

static const char *TAG = "WIFI_MANAGER";
static bool wifi_connected = false;   // Estado de la conexión Wi-Fi

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
				esp_wifi_connect();	// Reconexion automatica
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

	// Inicialización del almacenamiento en NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		nvs_flash_erase();
		nvs_flash_init();
	}

	// Inicialización de la pila de red
	esp_netif_init();
	// Creación del bucle de eventos por defecto
	esp_event_loop_create_default();

	// Configuración de la interfaz de red Wi-Fi en modo estación (STA)
	esp_netif_create_default_wifi_sta();

	// Inicialización del controlador Wi-Fi con la configuración por defecto
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);

	// Configuración de las credenciales Wi-Fi
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASS
		}
	};

	// Registro de Eventos:
	esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
	esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

	// Establecer el modo de operación en modo estación (STA)
	esp_wifi_set_mode(WIFI_MODE_STA);
	// Aplicar la configuración Wi-Fi (SSID y contraseña)
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	// Iniciar el Wi-Fi
	esp_wifi_start();

	// Confirmación por logs
	ESP_LOGI(TAG,"Wi-Fi inicializado, conectándose a %s", WIFI_SSID );

}
