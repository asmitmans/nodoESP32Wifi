idf_component_register(SRCS "task_sensor.c"
                            "task_http_post.c"
                    INCLUDE_DIRS "."
                    REQUIRES dht22 sensor_manager http_client)