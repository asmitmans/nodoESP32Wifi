#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

void wifi_init(void);
bool is_wifi_connected(void);

#endif // WIFI_MANAGER_H
