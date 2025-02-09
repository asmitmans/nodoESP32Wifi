idf_component_register(SRCS "sensor_manager.c" INCLUDE_DIRS "." REQUIRES 
	dht22 tasks wifi_manager ntp_client http_client esp_timer nvs_storage)
