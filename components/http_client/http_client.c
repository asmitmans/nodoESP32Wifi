#include "http_client.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "sdkconfig.h"

static const char *TAG = "HTTP_CLIENT";

void http_client_init(void) {
    ESP_LOGI(TAG, "Cliente HTTP inicializado");
}

bool http_client_post(const char* json_data) {
    esp_http_client_config_t config = {
        .url = CONFIG_HTTP_POST_URL,
        .timeout_ms = CONFIG_HTTP_POST_TIMEOUT,
        .method = HTTP_METHOD_POST
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Datos enviados. Código HTTP: %d", status_code);

        // Validar si el código HTTP es éxito (200-299)
        if (status_code >= 200 && status_code < 300) {
            esp_http_client_cleanup(client);
            return true;
        } else {
            ESP_LOGE(TAG, "Error en respuesta HTTP. Código: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "Error al enviar datos: %s", esp_err_to_name(err));
    }

    // Si falló, limpiar y retornar falso
    esp_http_client_cleanup(client);
    return false;
}
